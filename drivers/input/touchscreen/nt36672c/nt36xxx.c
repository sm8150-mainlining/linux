
/*
 * Copyright (C) 2010 - 2018 Novatek, Inc.
 *
 * $Revision: 32206 $
 * $Date: 2018-08-10 19:23:04 +0800 (週五, 10 八月 2018) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/debugfs.h>
#include <linux/pinctrl/consumer.h>
#include <linux/init.h>

#include <drm/drm_panel.h>


#include "nt36xxx.h"

#include <linux/jiffies.h>

static struct drm_panel_follower_funcs nvt_ts_follower_funcs;

static struct delayed_work nvt_esd_check_work;
static struct workqueue_struct *nvt_esd_check_wq;
static unsigned long irq_timer = 0;
uint8_t esd_check = false;
uint8_t esd_retry = 0;
static int esd_check_force = 0;
static int esd_check_scale = 8;

struct nvt_ts_data *ts;

#ifndef MODULE
static struct workqueue_struct *nvt_fwu_wq;
static struct workqueue_struct *nvt_lockdown_wq;
extern void Boot_Update_Firmware(struct work_struct *work);
#endif

static int32_t nvt_ts_suspend(struct device *dev);
static int32_t nvt_ts_resume(struct device *dev);
extern void dsi_panel_doubleclick_enable(bool on);
uint32_t ENG_RST_ADDR  = 0x7FFF80;
uint32_t SWRST_N8_ADDR = 0; /* read from dtsi */
uint32_t SPI_RD_FAST_ADDR = 0; /* read from dtsi */

static uint8_t bTouchIsAwake = 0;
/*******************************************************
Description:
	Novatek touchscreen irq enable/disable function.
return:
	n.a.
*******************************************************/
static void nvt_irq_enable(bool enable)
{
	if (enable) {
		if (!ts->irq_enabled) {
			enable_irq(ts->client->irq);
			ts->irq_enabled = true;
		}
	} else {
		if (ts->irq_enabled) {
			disable_irq(ts->client->irq);
			ts->irq_enabled = false;
		}
	}
}

/*******************************************************
Description:
	Novatek touchscreen spi read/write core function.
return:
	Executive outcomes. 0---succeed.
*******************************************************/
static inline int32_t spi_read_write(struct spi_device *client, uint8_t *buf, size_t len , NVT_SPI_RW rw)
{
	struct spi_message m;
	struct spi_transfer t = {
		.len    = len,
	};

	memcpy(ts->xbuf, buf, len);

	switch (rw) {
		case NVTREAD:
			t.tx_buf = ts->xbuf;
			t.rx_buf = ts->rbuf;
			t.len    = (len + DUMMY_BYTES);
			break;

		case NVTWRITE:
			t.tx_buf = ts->xbuf;
			break;
	}
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(client, &m);
}

/*******************************************************
Description:
	Novatek touchscreen spi read function.
return:
	Executive outcomes. 2---succeed. -5---I/O error
*******************************************************/
int32_t CTP_SPI_READ(struct spi_device *client, uint8_t *buf, uint16_t len)
{
	int32_t ret = -1;
	int32_t retries = 0;

	mutex_lock(&ts->xbuf_lock);

	buf[0] = SPI_READ_MASK(buf[0]);

	while (retries < 5) {
		ret = spi_read_write(client, buf, len, NVTREAD);
		if (ret == 0) break;
		retries++;
	}

	if (unlikely(retries == 5)) {
		NVT_ERR("read error, ret=%d\n", ret);
		ret = -EIO;
	} else {
		memcpy((buf+1), (ts->rbuf+2), (len-1));
	}

	mutex_unlock(&ts->xbuf_lock);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen spi write function.
return:
	Executive outcomes. 1---succeed. -5---I/O error
*******************************************************/
int32_t CTP_SPI_WRITE(struct spi_device *client, uint8_t *buf, uint16_t len)
{
	int32_t ret = -1;
	int32_t retries = 0;
	mutex_lock(&ts->xbuf_lock);
	buf[0] = SPI_WRITE_MASK(buf[0]);
	while (retries < 5) {
		ret = spi_read_write(client, buf, len, NVTWRITE);
		if (ret == 0)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
		NVT_ERR("error, ret=%d\n", ret);
		ret = -EIO;
	}
	mutex_unlock(&ts->xbuf_lock);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen set index/page/addr address.
return:
	Executive outcomes. 0---succeed. -5---access fail.
*******************************************************/
int32_t nvt_set_page(uint32_t addr)
{
	uint8_t buf[4] = {0};

	buf[0] = 0xFF;	/* set index/page/addr command */
	buf[1] = (addr >> 15) & 0xFF;
	buf[2] = (addr >> 7) & 0xFF;

	return CTP_SPI_WRITE(ts->client, buf, 3);
}

/*******************************************************
Description:
	Novatek touchscreen write data to specify address.
return:
	Executive outcomes. 0---succeed. -5---access fail.
*******************************************************/
int32_t nvt_write_addr(uint32_t addr, uint8_t data)
{
	int32_t ret = 0;
	uint8_t buf[4] = {0};
	NVT_LOG("nvt_write_addr enter\n");
	/* ---set xdata index--- */
	buf[0] = 0xFF;	/* set index/page/addr command */
	buf[1] = (addr >> 15) & 0xFF;
	buf[2] = (addr >> 7) & 0xFF;
	ret = CTP_SPI_WRITE(ts->client, buf, 3);
	if (ret) {
		NVT_ERR("set page 0x%06X failed, ret = %d\n", addr, ret);
		return ret;
	}

	/* ---write data to index--- */
	buf[0] = addr & (0x7F);
	buf[1] = data;
	ret = CTP_SPI_WRITE(ts->client, buf, 2);
	if (ret) {
		NVT_ERR("write data to 0x%06X failed, ret = %d\n", addr, ret);
		return ret;
	}

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen enable hw bld crc function.
return:
	N/A.
*******************************************************/
void nvt_bld_crc_enable(void)
{
	uint8_t buf[2] = {0};

	/* ---set xdata index to BLD_CRC_EN_ADDR--- */
	nvt_set_page(ts->mmap->BLD_CRC_EN_ADDR);

	/* ---read data from index--- */
	buf[0] = ts->mmap->BLD_CRC_EN_ADDR & (0x7F);
	buf[1] = 0xFF;
	CTP_SPI_READ(ts->client, buf, 2);

	/* ---write data to index--- */
	buf[0] = ts->mmap->BLD_CRC_EN_ADDR & (0x7F);
	buf[1] = buf[1] | (0x01 << 7);
	CTP_SPI_WRITE(ts->client, buf, 2);
}

/*******************************************************
Description:
	Novatek touchscreen clear status & enable fw crc function.
return:
	N/A.
*******************************************************/
void nvt_fw_crc_enable(void)
{
	uint8_t buf[2] = {0};

	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(ts->mmap->EVENT_BUF_ADDR);

	/* ---clear fw reset status--- */
	buf[0] = EVENT_MAP_RESET_COMPLETE & (0x7F);
	buf[1] = 0x00;
	CTP_SPI_WRITE(ts->client, buf, 2);

	/* ---enable fw crc--- */
	buf[0] = EVENT_MAP_HOST_CMD & (0x7F);
	buf[1] = 0xAE;	/* enable fw crc command */
	CTP_SPI_WRITE(ts->client, buf, 2);
}

/*******************************************************
Description:
	Novatek touchscreen set boot ready function.
return:
	N/A.
*******************************************************/
void nvt_boot_ready(void)
{
	/* ---write BOOT_RDY status cmds--- */
	nvt_write_addr(ts->mmap->BOOT_RDY_ADDR, 1);

	mdelay(5);

	if (!ts->hw_crc) {
		/* ---write BOOT_RDY status cmds--- */
		nvt_write_addr(ts->mmap->BOOT_RDY_ADDR, 0);

		/* ---write POR_CD cmds--- */
		nvt_write_addr(ts->mmap->POR_CD_ADDR, 0xA0);
	}
}

/*******************************************************
Description:
	Novatek touchscreen eng reset cmd
	function.
return:
	n.a.
*******************************************************/
void nvt_eng_reset(void)
{
	/* ---eng reset cmds to ENG_RST_ADDR--- */
	NVT_LOG("%s \n", __func__);
	nvt_write_addr(ENG_RST_ADDR, 0x5A);
	NVT_LOG("%s leave\n", __func__);
	mdelay(1);	/* wait tMCU_Idle2TP_REX_Hi after TP_RST */
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU
	function.
return:
	n.a.
*******************************************************/
void nvt_sw_reset(void)
{
	/* ---software reset cmds to SWRST_N8_ADDR--- */
	nvt_write_addr(SWRST_N8_ADDR, 0x55);

	msleep(10);
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU then into idle mode
	function.
return:
	n.a.
*******************************************************/
void nvt_sw_reset_idle(void)
{
	/* ---MCU idle cmds to SWRST_N8_ADDR--- */
	nvt_write_addr(SWRST_N8_ADDR, 0xAA);

	msleep(15);
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU (boot) function.
return:
	n.a.
*******************************************************/
void nvt_bootloader_reset(void)
{
	/* ---reset cmds to SWRST_N8_ADDR--- */
	nvt_write_addr(SWRST_N8_ADDR, 0x69);

	mdelay(5); /* wait tBRST2FR after Bootload RST */

	if (SPI_RD_FAST_ADDR) {
		/* disable SPI_RD_FAST */
		nvt_write_addr(SPI_RD_FAST_ADDR, 0x00);
	}
}

/*******************************************************
Description:
	Novatek touchscreen clear FW status function.
return:
	Executive outcomes. 0---succeed. -1---fail.
*******************************************************/
int32_t nvt_clear_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 20;

	for (i = 0; i < retry; i++) {
		/* ---set xdata index to EVENT BUF ADDR--- */
		nvt_set_page(ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE);

		/* ---clear fw status--- */
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0x00;
		CTP_SPI_WRITE(ts->client, buf, 2);

		/* ---read fw status--- */
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0xFF;
		CTP_SPI_READ(ts->client, buf, 2);

		if (buf[1] == 0x00)
			break;

		usleep_range(10000, 10000);
	}

	if (i >= retry) {
		NVT_ERR("failed, i=%d, buf[1]=0x%02X\n", i, buf[1]);
		return -1;
	} else {
		return 0;
	}
}

/*******************************************************
Description:
	Novatek touchscreen check FW status function.
return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_check_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 50;

	for (i = 0; i < retry; i++) {
		/* ---set xdata index to EVENT BUF ADDR--- */
		nvt_set_page(ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE);

		/* ---read fw status--- */
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0x00;
		CTP_SPI_READ(ts->client, buf, 2);

		if ((buf[1] & 0xF0) == 0xA0)
			break;

		usleep_range(10000, 10000);
	}

	if (i >= retry) {
		NVT_ERR("failed, i=%d, buf[1]=0x%02X\n", i, buf[1]);
		return -1;
	} else {
		return 0;
	}
}

/*******************************************************
Description:
	Novatek touchscreen check FW reset state function.
return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;
	int32_t retry = 0;
	int32_t retry_max = (check_reset_state == RESET_STATE_INIT) ? 10 : 50;

	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_RESET_COMPLETE);

	while (1) {
		/* ---read reset state--- */
		buf[0] = EVENT_MAP_RESET_COMPLETE;
		buf[1] = 0x00;
		CTP_SPI_READ(ts->client, buf, 6);

		if ((buf[1] >= check_reset_state) && (buf[1] <= RESET_STATE_MAX)) {
			ret = 0;
			break;
		}

		retry++;
		if(unlikely(retry > retry_max)) {
			NVT_ERR("error, retry=%d, buf[1]=0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
				retry, buf[1], buf[2], buf[3], buf[4], buf[5]);
			ret = -1;
			break;
		}

		usleep_range(10000, 10000);
	}

	return ret;
}

void nvt_esd_check_enable(uint8_t enable)
{
	enable = esd_check_force;
	/* update interrupt timer */
	irq_timer = jiffies;
	/* clear esd_retry counter, if protect function is enabled */
	esd_retry = enable ? 0 : esd_retry;
	/* enable/disable esd check flag */
	esd_check = enable;
}

/*******************************************************
Description:
	Novatek touchscreen get novatek project id information
	function.
return:
	Executive outcomes. 0---success. -1---fail.
*******************************************************/
int32_t nvt_read_pid(void)
{
	uint8_t buf[3] = {0};
	int32_t ret = 0;

	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_PROJECTID);

	/* ---read project id--- */
	buf[0] = EVENT_MAP_PROJECTID;
	buf[1] = 0x00;
	buf[2] = 0x00;
	CTP_SPI_READ(ts->client, buf, 3);

	ts->nvt_pid = (buf[2] << 8) + buf[1];

	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(ts->mmap->EVENT_BUF_ADDR);

	NVT_LOG("PID=%04X\n", ts->nvt_pid);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen get firmware related information
	function.
return:
	Executive outcomes. 0---success. -1---fail.
*******************************************************/
int32_t nvt_get_fw_info(void)
{
	uint8_t buf[64] = {0};
	uint32_t retry_count = 0;
	int32_t ret = 0;

#if 0
	/* READ CHIP ID */
	/* ---set xdata index to 0x1F600-- */
	nvt_set_page(0x1F600);
	buf[0] = 0x4E;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x00;
	CTP_SPI_READ(ts->client, buf, 7);
	NVT_LOG("buf[1]=0x%02X, buf[2]=0x%02X, buf[3]=0x%02X, buf[4]=0x%02X, buf[5]=0x%02X, buf[6]=0x%02X\n",
		buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
	memset(buf, 0, 64);
#endif
info_retry:
	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_FWINFO);

	/* ---read fw info--- */
	buf[0] = EVENT_MAP_FWINFO;
	CTP_SPI_READ(ts->client, buf, 17);
	ts->fw_ver = buf[1];
	ts->x_num = buf[3];
	ts->y_num = buf[4];
	ts->abs_x_max = (uint16_t)((buf[5] << 8) | buf[6]);
	ts->abs_y_max = (uint16_t)((buf[7] << 8) | buf[8]);
	ts->max_button_num = buf[11];

	/* ---clear x_num, y_num if fw info is broken--- */
	if ((buf[1] + buf[2]) != 0xFF) {
		NVT_ERR("FW info is broken! fw_ver=0x%02X, ~fw_ver=0x%02X\n", buf[1], buf[2]);
		ts->fw_ver = 0;
		ts->x_num = 18;
		ts->y_num = 32;
		ts->abs_x_max = TOUCH_DEFAULT_MAX_WIDTH;
		ts->abs_y_max = TOUCH_DEFAULT_MAX_HEIGHT;
		ts->max_button_num = TOUCH_KEY_NUM;

		if(retry_count < 3) {
			retry_count++;
			NVT_ERR("retry_count=%d\n", retry_count);
			goto info_retry;
		} else {
			NVT_ERR("Set default fw_ver=%d, x_num=%d, y_num=%d, "
					"abs_x_max=%d, abs_y_max=%d, max_button_num=%d!\n",
					ts->fw_ver, ts->x_num, ts->y_num,
					ts->abs_x_max, ts->abs_y_max, ts->max_button_num);
			ret = -1;
		}
	} else {
		ret = 0;
	}

	NVT_LOG("FW type is 0x%02X, fw_ver=%d\n", buf[14], ts->fw_ver);

	/* ---Get Novatek PID--- */
	nvt_read_pid();
	return ret;
}

/*******************************************************
  Create Device Node (Proc Entry)
*******************************************************/
static struct proc_dir_entry *NVT_proc_entry;
#define DEVICE_NAME	"NVTSPI"

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTSPI read function.
return:
	Executive outcomes. 2---succeed. -5,-14---failed.
*******************************************************/
static ssize_t nvt_flash_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
	uint8_t *str = NULL;
	int32_t ret = 0;
	int32_t retries = 0;
	int8_t spi_wr = 0;
	uint8_t *buf;

	if (count > NVT_TRANSFER_LEN) {
		NVT_ERR("invalid transfer len!\n");
		return -EFAULT;
	}

	/* allocate buffer for spi transfer */
	str = (uint8_t *)kzalloc((count), GFP_KERNEL);
	if(str == NULL) {
		NVT_ERR("kzalloc for buf failed!\n");
		ret = -ENOMEM;
		goto kzalloc_failed;
	}

	buf = (uint8_t *)kzalloc((count), GFP_KERNEL | GFP_DMA);
	if(buf == NULL) {
		NVT_ERR("kzalloc for buf failed!\n");
		ret = -ENOMEM;
		kfree(str);
		str = NULL;
		goto kzalloc_failed;
	}

	if (copy_from_user(str, buff, count)) {
		NVT_ERR("copy from user error\n");
		ret = -EFAULT;
		goto out;
	}

	/*
	 * stop esd check work to avoid case that 0x77 report righ after here to enable esd check again
	 * finally lead to trigger esd recovery bootloader reset
	 */
	cancel_delayed_work_sync(&nvt_esd_check_work);
	nvt_esd_check_enable(false);

	spi_wr = str[0] >> 7;
	memcpy(buf, str+2, ((str[0] & 0x7F) << 8) | str[1]);

	if (spi_wr == NVTWRITE) { /* SPI write */
		while (retries < 20) {
			ret = CTP_SPI_WRITE(ts->client, buf, ((str[0] & 0x7F) << 8) | str[1]);
			if (!ret)
				break;
			else
				NVT_ERR("error, retries=%d, ret=%d\n", retries, ret);

			retries++;
		}

		if (unlikely(retries == 20)) {
			NVT_ERR("error, ret = %d\n", ret);
			ret = -EIO;
			goto out;
		}
	} else if (spi_wr == NVTREAD) { /* SPI read */
		while (retries < 20) {
			ret = CTP_SPI_READ(ts->client, buf, ((str[0] & 0x7F) << 8) | str[1]);
			if (!ret)
				break;
			else
				NVT_ERR("error, retries=%d, ret=%d\n", retries, ret);

			retries++;
		}

		memcpy(str+2, buf, ((str[0] & 0x7F) << 8) | str[1]);
		/* copy buff to user if spi transfer */
		if (retries < 20) {
			if (copy_to_user(buff, str, count)) {
				ret = -EFAULT;
				goto out;
			}
		}

		if (unlikely(retries == 20)) {
			NVT_ERR("error, ret = %d\n", ret);
			ret = -EIO;
			goto out;
		}
	} else {
		NVT_ERR("Call error, str[0]=%d\n", str[0]);
		ret = -EFAULT;
		goto out;
	}

out:
	kfree(str);
	kfree(buf);
kzalloc_failed:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTSPI open function.
return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
static int32_t nvt_flash_open(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev;

	dev = kmalloc(sizeof(struct nvt_flash_data), GFP_KERNEL);
	if (dev == NULL) {
		NVT_ERR("Failed to allocate memory for nvt flash data\n");
		return -ENOMEM;
	}

	rwlock_init(&dev->lock);
	file->private_data = dev;

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTSPI close function.
return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_flash_close(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev = file->private_data;

	if (dev)
		kfree(dev);

	return 0;
}

static const struct file_operations nvt_flash_fops = {
	//.proc_owner = THIS_MODULE,
	.open = nvt_flash_open,
	.release = nvt_flash_close,
	.read = nvt_flash_read,
};

static const struct proc_ops nvt_flash_pfops = {
	//.proc_owner = THIS_MODULE,
	.proc_open = nvt_flash_open,
	.proc_release = nvt_flash_close,
	.proc_read = nvt_flash_read,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTSPI initial function.
return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
static int32_t nvt_flash_proc_init(void)
{
	NVT_proc_entry = proc_create(DEVICE_NAME, 0444, NULL,&nvt_flash_pfops);
	if (NVT_proc_entry == NULL) {
		NVT_ERR("Failed!\n");
		return -ENOMEM;
	} else {
		NVT_LOG("Succeeded!\n");
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTSPI deinitial function.
return:
	n.a.
*******************************************************/
static void nvt_flash_proc_deinit(void)
{
	if (NVT_proc_entry != NULL) {
		remove_proc_entry(DEVICE_NAME, NULL);
		NVT_proc_entry = NULL;
		NVT_LOG("Removed /proc/%s\n", DEVICE_NAME);
	}
}

/*******************************************************
Description:
	Novatek touchscreen parse device tree function.
return:
	n.a.
*******************************************************/
#ifdef CONFIG_OF
static int32_t nvt_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int32_t ret = 0;

	ts->reset_gpio = of_get_named_gpio(np, "novatek,reset-gpio", 0);
	NVT_LOG("novatek,reset-gpio=%d\n", ts->reset_gpio);
	ts->irq_gpio = of_get_named_gpio(np, "novatek,irq-gpio", 0);
	NVT_LOG("novatek,irq-gpio=%d\n", ts->irq_gpio);

	ret = of_property_read_u32(np, "novatek,swrst-n8-addr", &SWRST_N8_ADDR);
	if (ret) {
		NVT_ERR("error reading novatek,swrst-n8-addr. ret=%d\n", ret);
		return ret;
	} else {
		NVT_LOG("SWRST_N8_ADDR=0x%06X\n", SWRST_N8_ADDR);
	}

	ret = of_property_read_u32(np, "novatek,spi-rd-fast-addr", &SPI_RD_FAST_ADDR);
	if (ret) {
		NVT_ERR("not support novatek,spi-rd-fast-addr\n");
		SPI_RD_FAST_ADDR = 0;
		ret = 0;
	} else {
		NVT_LOG("SPI_RD_FAST_ADDR=0x%06X\n", SPI_RD_FAST_ADDR);
	}

	ret = of_property_read_u32(np, "spi-max-frequency", &ts->spi_max_freq);
	if (ret) {
		NVT_ERR("Unable to get spi freq\n");
		return ret;
	} else {
		NVT_LOG("spi-max-frequency: %u\n", ts->spi_max_freq);
	}

	// Allow firmware to be overwritten by device tree
        ret = of_property_read_string(np, "firmware-name", &ts->firmware_name);
        if (ret) {
                NVT_ERR("Unable to get firmware name, Please specify a firmware via fimware-name property in DT!");
                return ret;
        } else {
                NVT_LOG("firmware-name: %s\n", ts->firmware_name);
        }

	return ret;
}
#endif

/*******************************************************
Description:
	Novatek touchscreen config and request gpio
return:
	Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int nvt_gpio_config(struct nvt_ts_data *ts)
{
	int32_t ret = 0;

	/* request RST-pin (Output/High) */
	if (gpio_is_valid(ts->reset_gpio)) {
		ret = gpio_request_one(ts->reset_gpio, GPIOF_OUT_INIT_LOW, "NVT-tp-rst");
		if (ret) {
			NVT_ERR("Failed to request NVT-tp-rst GPIO\n");
			goto err_request_reset_gpio;
		}
	}

	/* request INT-pin (Input) */
	if (gpio_is_valid(ts->irq_gpio)) {
		ret = gpio_request_one(ts->irq_gpio, GPIOF_IN, "NVT-int");
		if (ret) {
			NVT_ERR("Failed to request NVT-int GPIO\n");
			goto err_request_irq_gpio;
		}
	}
	NVT_LOG("%s Exit\n", __func__);
	return ret;

err_request_irq_gpio:
	gpio_free(ts->reset_gpio);
err_request_reset_gpio:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen deconfig gpio
return:
	n.a.
*******************************************************/
static void nvt_gpio_deconfig(struct nvt_ts_data *ts)
{
	if (gpio_is_valid(ts->irq_gpio))
		gpio_free(ts->irq_gpio);
	if (gpio_is_valid(ts->reset_gpio))
		gpio_free(ts->reset_gpio);
}

module_param_named(esd_check_force, esd_check_force, int, 0664);

static uint8_t nvt_fw_recovery(uint8_t *point_data)
{
	uint8_t i = 0;
	uint8_t detected = true;

	/* check pattern */
	for (i=1 ; i<7 ; i++) {
		if (point_data[i] != 0x77) {
			detected = false;
			break;
		}
	}

	return detected;
}

module_param_named(esd_check_scale, esd_check_scale, int, 0664);
static void nvt_esd_check_func(struct work_struct *work)
{
	unsigned int timer = jiffies_to_msecs(jiffies - irq_timer);

	if (esd_check_scale > 24)
		esd_check_scale = 24;
	if (esd_check_scale < 4)
		esd_check_scale = 4;

	if ((timer > esd_check_scale * 125 + 50)
		&& (timer < 2 * esd_check_scale * 125 - 50) && esd_check) {
		mutex_lock(&ts->lock);
		NVT_LOG("do ESD recovery, timer = %d, retry = %d\n", timer, esd_retry);
		/* do esd recovery, reload fw */
		nvt_update_firmware(ts->firmware_name);
		mutex_unlock(&ts->lock);
		/* update interrupt timer */
		irq_timer = jiffies;
		/* update esd_retry counter */
		esd_retry++;
	}

	queue_delayed_work(nvt_esd_check_wq, &nvt_esd_check_work,
			msecs_to_jiffies(125));
}

static uint8_t recovery_cnt = 0;
static uint8_t nvt_wdt_fw_recovery(uint8_t *point_data)
{
	uint32_t recovery_cnt_max = 10;
	uint8_t recovery_enable = false;
	uint8_t i = 0;

	recovery_cnt++;

	/* check pattern */
	for (i=1 ; i<7 ; i++) {
		if ((point_data[i] != 0xFD) && (point_data[i] != 0xFE)) {
			recovery_cnt = 0;
			break;
		}
	}

	if (recovery_cnt > recovery_cnt_max){
		recovery_enable = true;
		recovery_cnt = 0;
	}

	return recovery_enable;
}

static uint32_t nvt_dump_fw_history(void)
{
	int32_t ret = 0;
	uint8_t buf[FW_HISTORY_SIZE + 1 + DUMMY_BYTES] = {0};
	int32_t i = 0;
	char *tmp_dump = NULL;
	int32_t line_cnt = 0;

	if (ts->mmap->FW_HISTORY_ADDR == 0) {
		NVT_ERR("FW_HISTORY_ADDR not available!\n");
		ret = -1;
		goto exit_nvt_dump_fw_history;
	}
	nvt_set_page(ts->mmap->FW_HISTORY_ADDR);
	buf[0] = ts->mmap->FW_HISTORY_ADDR & 0xFF;
	CTP_SPI_READ(ts->client, buf, FW_HISTORY_SIZE + 1);
	if (ret) {
		NVT_ERR("CTP_SPI_READ failed.(%d)\n", ret);
		ret = -1;
		goto exit_nvt_dump_fw_history;
		}

	tmp_dump = (char *)kzalloc(FW_HISTORY_SIZE * 4, GFP_KERNEL);
	for (i = 0; i < FW_HISTORY_SIZE; i++) {
		sprintf(tmp_dump + i * 3 + line_cnt, "%02X ", buf[1 + i]);
		if ((i + 1) % 16 == 0) {
			sprintf(tmp_dump + i * 3 + line_cnt + 3, "%c", '\n');
			line_cnt++;
		}
	}
	NVT_LOG("%s", tmp_dump);

exit_nvt_dump_fw_history:
	if (tmp_dump) {
		kfree(tmp_dump);
		tmp_dump = NULL;
	}
	nvt_set_page(ts->mmap->EVENT_BUF_ADDR);

	return ret;
}

#define POINT_DATA_LEN 65
/*******************************************************
Description:
	Novatek touchscreen work function.
return:
	n.a.
*******************************************************/
static irqreturn_t nvt_ts_work_func(int irq, void *data)
{
	int32_t ret = -1;
	uint8_t point_data[POINT_DATA_LEN + 1 + DUMMY_BYTES] = {0};
	uint32_t position = 0;
	uint32_t input_x = 0;
	uint32_t input_y = 0;
	uint32_t input_w = 0;
	uint32_t input_p = 0;
	uint8_t input_id = 0;
	uint8_t press_id[TOUCH_MAX_FINGER_NUM] = {0};
	int32_t i = 0;
	int32_t finger_cnt = 0;

	mutex_lock(&ts->lock);
	if (ts->dev_pm_suspend) {
		ret = wait_for_completion_timeout(&ts->dev_pm_suspend_completion, msecs_to_jiffies(500));
		if (!ret) {
			NVT_ERR("system(spi) can't finished resuming procedure, skip it\n");
			goto XFER_ERROR;
		}
	}

	ret = CTP_SPI_READ(ts->client, point_data, POINT_DATA_LEN + 1);
	if (ret < 0) {
		NVT_ERR("CTP_SPI_READ failed.(%d)\n", ret);
		goto XFER_ERROR;
	}

	/* ESD protect by WDT */
	if (nvt_wdt_fw_recovery(point_data)) {
		NVT_ERR("Recover for fw reset, %02X\n", point_data[1]);
		if (point_data[1] == 0xFD) {
			NVT_ERR("Dump FW history:\n");
			nvt_dump_fw_history();
		}
		nvt_update_firmware(ts->firmware_name);
		goto XFER_ERROR;
   }

	/* ESD protect by FW handshake */
	if (nvt_fw_recovery(point_data)) {
		nvt_esd_check_enable(true);
		goto XFER_ERROR;
	}

	finger_cnt = 0;

	for (i = 0; i < ts->max_touch_num; i++) {
		position = 1 + 6 * i;
		input_id = (uint8_t)(point_data[position + 0] >> 3);
		if ((input_id == 0) || (input_id > ts->max_touch_num))
			continue;

		if (((point_data[position] & 0x07) == 0x01) || ((point_data[position] & 0x07) == 0x02)) {	//finger down (enter & moving)
			/* update interrupt timer */
			irq_timer = jiffies;
			input_x = (uint32_t)(point_data[position + 1] << 4) + (uint32_t) (point_data[position + 3] >> 4);
			input_y = (uint32_t)(point_data[position + 2] << 4) + (uint32_t) (point_data[position + 3] & 0x0F);
			if ((input_x < 0) || (input_y < 0))
				continue;
			if ((input_x > ts->abs_x_max) || (input_y > ts->abs_y_max))
				continue;
			input_w = (uint32_t)(point_data[position + 4]);
			if (input_w == 0)
				input_w = 1;
			if (i < 2) {
				input_p = (uint32_t)(point_data[position + 5]) + (uint32_t)(point_data[i + 63] << 8);
				if (input_p > TOUCH_FORCE_NUM)
					input_p = TOUCH_FORCE_NUM;
			} else {
				input_p = (uint32_t)(point_data[position + 5]);
			}
			if (input_p == 0)
				input_p = 1;

			press_id[input_id - 1] = 1;
			input_mt_slot(ts->input_dev, input_id - 1);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_report_key(ts->input_dev, BTN_TOOL_FINGER, 1);

			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);

			set_bit(input_id - 1, ts->slot_map);
			finger_cnt++;
		}
	}

	for (i = 0; i < ts->max_touch_num; i++) {
		if (press_id[i] != 1) {
			input_mt_slot(ts->input_dev, i);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
			if (finger_cnt == 0 && test_bit(i, ts->slot_map)) {
				input_report_key(ts->input_dev, BTN_TOUCH, 0);
				input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
			}
			clear_bit(i, ts->slot_map);
		}
	}

	input_sync(ts->input_dev);

XFER_ERROR:
	mutex_unlock(&ts->lock);
	return IRQ_HANDLED;
}


/*******************************************************
Description:
	Novatek touchscreen check chip version trim function.
return:
	Executive outcomes. 0---NVT IC. -1---not NVT IC.
*******************************************************/
static int8_t nvt_ts_check_chip_ver_trim(void)
{
	uint8_t buf[8] = {0};
	int32_t retry = 0;
	int32_t list = 0;
	int32_t i = 0;
	int32_t found_nvt_chip = 0;
	int32_t ret = -1;

	/* ---Check for 5 times--- */
	for (retry = 5; retry > 0; retry--) {

		nvt_bootloader_reset();

		/* ---set xdata index to 0x1F600--- */
		nvt_set_page(0x1F600);

		buf[0] = 0x4E;
		buf[1] = 0x00;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = 0x00;
		CTP_SPI_READ(ts->client, buf, 7);
		NVT_LOG("buf[1]=0x%02X, buf[2]=0x%02X, buf[3]=0x%02X, buf[4]=0x%02X, buf[5]=0x%02X, buf[6]=0x%02X\n",
			buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

		/* compare read chip id on supported list */
		for (list = 0; list < (sizeof(trim_id_table) / sizeof(struct nvt_ts_trim_id_table)); list++) {
			found_nvt_chip = 0;
			/* compare each byte */
			for (i = 0; i < NVT_ID_BYTE_MAX; i++) {
				if (trim_id_table[list].mask[i]) {
					if (buf[i + 1] != trim_id_table[list].id[i])
						break;
				}
			}

			if (i == NVT_ID_BYTE_MAX) {
				found_nvt_chip = 1;
			}

			if (found_nvt_chip) {
				NVT_LOG("This is NVT touch IC\n");
				ts->mmap = trim_id_table[list].mmap;
				ts->carrier_system = trim_id_table[list].hwinfo->carrier_system;
				ts->hw_crc = trim_id_table[list].hwinfo->hw_crc;
				ret = 0;
				goto out;
			} else {
				ts->mmap = NULL;
				ret = -1;
			}
		}

		msleep(10);
	}

out:
	return ret;
}

static int nvt_pinctrl_init(struct nvt_ts_data *nvt_data)
{
	int retval = 0;
	/* Get pinctrl if target uses pinctrl */
	nvt_data->ts_pinctrl = devm_pinctrl_get(&nvt_data->client->dev);
	NVT_LOG("%s Enter\n", __func__);
	if (IS_ERR_OR_NULL(nvt_data->ts_pinctrl)) {
		retval = PTR_ERR(nvt_data->ts_pinctrl);
		NVT_ERR("Target does not use pinctrl %d\n", retval);
		goto err_pinctrl_get;
	}

	nvt_data->pinctrl_state_active
		= pinctrl_lookup_state(nvt_data->ts_pinctrl, PINCTRL_STATE_ACTIVE);

	if (IS_ERR_OR_NULL(nvt_data->pinctrl_state_active)) {
		retval = PTR_ERR(nvt_data->pinctrl_state_active);
		NVT_ERR("Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_ACTIVE, retval);
		goto err_pinctrl_lookup;
	}

	nvt_data->pinctrl_state_suspend
		= pinctrl_lookup_state(nvt_data->ts_pinctrl, PINCTRL_STATE_SUSPEND);

	if (IS_ERR_OR_NULL(nvt_data->pinctrl_state_suspend)) {
		retval = PTR_ERR(nvt_data->pinctrl_state_suspend);
		NVT_ERR("Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_SUSPEND, retval);
		goto err_pinctrl_lookup;
	}

	return 0;
err_pinctrl_lookup:
	devm_pinctrl_put(nvt_data->ts_pinctrl);
err_pinctrl_get:
	nvt_data->ts_pinctrl = NULL;
	return retval;
}

static void nvt_resume_work(struct work_struct *work)
{
	struct nvt_ts_data *ts_core = container_of(work, struct nvt_ts_data, resume_work);
	nvt_ts_resume(&ts_core->client->dev);
}

/*******************************************************
Description:
	Novatek touchscreen driver probe function.
return:
	Executive outcomes. 0---succeed. negative---failed
*******************************************************/
static int32_t nvt_ts_probe(struct spi_device *client)
{
	struct device *dev = &client->dev;
	int32_t ret = 0;

	NVT_LOG("start\n");

	ts = kzalloc(sizeof(struct nvt_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		NVT_ERR("failed to allocated memory for nvt ts data\n");
		return -ENOMEM;
	}
	ts->xbuf = (uint8_t *)kzalloc((NVT_TRANSFER_LEN+1), GFP_KERNEL);
	if(ts->xbuf == NULL) {
		NVT_ERR("kzalloc for xbuf failed!\n");
		if (ts) {
			kfree(ts);
			ts = NULL;
		}
		return -ENOMEM;
		//goto err_malloc_xbuf;
	}

        /* ---parse dts--- */
        ret = nvt_parse_dt(&client->dev);
        if (ret) {
                NVT_ERR("parse dt error\n");
                goto err_spi_setup;
        }

	ts->client = client;
	dev_set_drvdata(dev, ts);

	/* ---prepare for spi parameter--- */
	if (ts->client->controller->flags & SPI_CONTROLLER_HALF_DUPLEX) {
		NVT_ERR("Full duplex not supported by master\n");
		ret = -EIO;
		goto err_ckeck_full_duplex;
	}
	ts->client->bits_per_word = 8;
	ts->client->mode = SPI_MODE_0;
	ts->client->max_speed_hz = ts->spi_max_freq;

	ret = spi_setup(client);
	if (ret < 0) {
		NVT_ERR("Failed to perform SPI setup\n");
		goto err_spi_setup;
	}

	NVT_LOG("mode=%d, max_speed_hz=%d\n", ts->client->mode, ts->client->max_speed_hz);

	ret = nvt_pinctrl_init(ts);
	if (!ret && ts->ts_pinctrl) {
		ret = pinctrl_select_state(ts->ts_pinctrl, ts->pinctrl_state_active);

		if (ret < 0) {
			NVT_ERR("Failed to select %s pinstate %d\n",
				PINCTRL_STATE_ACTIVE, ret);
		}
	} else {
		NVT_ERR("Failed to init pinctrl\n");
	}

	NVT_LOG("Request GPIO\n");
	/* ---request and config GPIOs--- */
	ret = nvt_gpio_config(ts);
	if (ret) {
		NVT_ERR("gpio config error!\n");
		goto err_gpio_config_failed;
	}

	mutex_init(&ts->lock);
	mutex_init(&ts->xbuf_lock);

	/* ---eng reset before TP_RESX high */

	nvt_eng_reset();
	gpio_set_value(ts->reset_gpio, 1);
	NVT_LOG("gpio set complete\n");
	/* need 10ms delay after POR(power on reset) */
	msleep(10);

	/* ---check chip version trim--- */
	NVT_LOG("start check chip\n");
	ret = nvt_ts_check_chip_ver_trim();
	if (ret) {
		NVT_ERR("chip is not identified\n");
		ret = -EINVAL;
		goto err_chipvertrim_failed;
	}
	NVT_LOG("finish check chip\n");

	ts->abs_x_max = TOUCH_DEFAULT_MAX_WIDTH;
	ts->abs_y_max = TOUCH_DEFAULT_MAX_HEIGHT;
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		NVT_ERR("allocate input device failed\n");
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}

	ts->max_touch_num = TOUCH_MAX_FINGER_NUM;


	ts->int_trigger_type = INT_TRIGGER_TYPE;

	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	__set_bit(BTN_TOUCH, ts->input_dev->keybit);
	__set_bit(BTN_TOOL_FINGER, ts->input_dev->keybit);
	ts->input_dev->propbit[0] = BIT(INPUT_PROP_DIRECT);

	input_mt_init_slots(ts->input_dev, ts->max_touch_num, 0);

#if TOUCH_MAX_FINGER_NUM > 1
	/*input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);*/

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max - 1, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max - 1, 0, 0);
	/* no need to set ABS_MT_TRACKING_ID, input_mt_init_slots() already set it */
#endif

	sprintf(ts->phys, "input/ts");
	ts->input_dev->name = NVT_TS_NAME;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_SPI;

	input_set_drvdata(ts->input_dev, ts);
	ret = input_register_device(ts->input_dev);
	if (ret) {
		NVT_ERR("register input device (%s) failed. ret=%d\n", ts->input_dev->name, ret);
		goto err_input_register_device_failed;
	}

	ts->client->irq = gpio_to_irq(ts->irq_gpio);
	if (ts->client->irq) {
		NVT_LOG("int_trigger_type=%d\n", ts->int_trigger_type);
		ts->irq_enabled = true;
		ret = request_threaded_irq(ts->client->irq, NULL, nvt_ts_work_func,
				ts->int_trigger_type | IRQF_ONESHOT, NVT_SPI_NAME, ts);
		if (ret != 0) {
			NVT_ERR("request irq failed. ret=%d\n", ret);
			goto err_int_request_failed;
		} else {
			nvt_irq_enable(false);
			NVT_LOG("request irq %d succeed\n", ts->client->irq);
		}
	}

	ts->ic_state = NVT_IC_INIT;
	ts->dev_pm_suspend = false;
	init_completion(&ts->dev_pm_suspend_completion);

	// We dont need a firmware boot delay if the driver is a module
	#ifndef MODULE
	nvt_fwu_wq = alloc_workqueue("nvt_fwu_wq", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!nvt_fwu_wq) {
		NVT_ERR("nvt_fwu_wq create workqueue failed\n");
		ret = -ENOMEM;
		goto err_create_nvt_fwu_wq_failed;
	}
	INIT_DELAYED_WORK(&ts->nvt_fwu_work, Boot_Update_Firmware);
	/* please make sure boot update start after display reset(RESX) sequence */
	queue_delayed_work(nvt_fwu_wq, &ts->nvt_fwu_work, msecs_to_jiffies(10000));
	#endif

	#ifdef MODULE
	ret = nvt_update_firmware(ts->firmware_name);
	if (ret)
		NVT_ERR("download firmware failed\n");
	#endif

	NVT_LOG("NVT_TOUCH_ESD_PROTECT is on\n");
	INIT_DELAYED_WORK(&nvt_esd_check_work, nvt_esd_check_func);
	nvt_esd_check_wq = alloc_workqueue("nvt_esd_check_wq", WQ_MEM_RECLAIM, 1);
	if (!nvt_esd_check_wq) {
		NVT_ERR("nvt_esd_check_wq create workqueue failed\n");
		ret = -ENOMEM;
		goto err_create_nvt_esd_check_wq_failed;
	}
	queue_delayed_work(nvt_esd_check_wq, &nvt_esd_check_work,
			msecs_to_jiffies(125));

	ret = nvt_flash_proc_init();
	if (ret != 0) {
		NVT_ERR("nvt flash proc init failed. ret=%d\n", ret);
		goto err_flash_proc_init_failed;
	}
	ts->event_wq = alloc_workqueue("nvt-event-queue",
		WQ_UNBOUND | WQ_HIGHPRI | WQ_CPU_INTENSIVE, 1);
	if (!ts->event_wq) {
		NVT_ERR("Can not create work thread for suspend/resume!!");
		ret = -ENOMEM;
		goto err_alloc_failed;
	}
	INIT_WORK(&ts->resume_work, nvt_resume_work);
	/*INIT_WORK(&ts->suspend_work, nvt_suspend_work);*/

	/* If the device follows a DRM panel, configure panel follower */
	if (drm_is_panel_follower(dev)) {
		NVT_LOG("DRM PANEL\n");
		ts->panel_follower.funcs = &nvt_ts_follower_funcs;
		devm_drm_panel_add_follower(dev, &ts->panel_follower);
	}
	else {
		NVT_LOG(" NOT A DRM PANEL\n");
	}

	bTouchIsAwake = 1;
	NVT_LOG("end\n");

	nvt_irq_enable(true);

	return 0;

err_alloc_failed:
nvt_flash_proc_deinit();
err_flash_proc_init_failed:
	if (nvt_esd_check_wq) {
		cancel_delayed_work_sync(&nvt_esd_check_work);
		destroy_workqueue(nvt_esd_check_wq);
		nvt_esd_check_wq = NULL;
	}
err_create_nvt_esd_check_wq_failed:

#ifndef MODULE
	if (nvt_fwu_wq) {
		cancel_delayed_work_sync(&ts->nvt_fwu_work);
		destroy_workqueue(nvt_fwu_wq);
		nvt_fwu_wq = NULL;
	}
err_create_nvt_fwu_wq_failed:
	if (nvt_lockdown_wq) {
		cancel_delayed_work_sync(&ts->nvt_lockdown_work);
		destroy_workqueue(nvt_lockdown_wq);
		nvt_lockdown_wq = NULL;
	}
#endif
err_int_request_failed:
	input_unregister_device(ts->input_dev);
	ts->input_dev = NULL;
err_input_register_device_failed:
	if (ts->input_dev) {
		input_free_device(ts->input_dev);
		ts->input_dev = NULL;
	}
err_input_dev_alloc_failed:
err_chipvertrim_failed:
	mutex_destroy(&ts->xbuf_lock);
	mutex_destroy(&ts->lock);
	nvt_gpio_deconfig(ts);
err_gpio_config_failed:
err_spi_setup:
err_ckeck_full_duplex:
	spi_set_drvdata(ts->client, NULL);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen driver release function.
return:
	Executive outcomes. 0---succeed.
*******************************************************/
void nvt_ts_remove(struct spi_device *client)
{
	NVT_LOG("Removing driver...\n");

	nvt_flash_proc_deinit();

	if (nvt_esd_check_wq) {
		cancel_delayed_work_sync(&nvt_esd_check_work);
		nvt_esd_check_enable(false);
		destroy_workqueue(nvt_esd_check_wq);
		nvt_esd_check_wq = NULL;
	}

#ifndef MODULE
	if (nvt_fwu_wq) {
		cancel_delayed_work_sync(&ts->nvt_fwu_work);
		destroy_workqueue(nvt_fwu_wq);
		nvt_fwu_wq = NULL;
	}
#endif

	nvt_irq_enable(false);
	free_irq(ts->client->irq, ts);

	mutex_destroy(&ts->xbuf_lock);
	mutex_destroy(&ts->lock);

	nvt_gpio_deconfig(ts);

	if (ts->input_dev) {
		input_unregister_device(ts->input_dev);
		ts->input_dev = NULL;
	}

	spi_set_drvdata(ts->client, NULL);

	if (ts) {
		kfree(ts);
		ts = NULL;
	}

	return;
}

static void nvt_ts_shutdown(struct spi_device *client)
{
	NVT_LOG("Shutdown driver...\n");

	nvt_irq_enable(false);

	nvt_flash_proc_deinit();

	if (nvt_esd_check_wq) {
		cancel_delayed_work_sync(&nvt_esd_check_work);
		nvt_esd_check_enable(false);
		destroy_workqueue(nvt_esd_check_wq);
		nvt_esd_check_wq = NULL;
	}
}

/*******************************************************
Description:
	Novatek touchscreen driver suspend function.
return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_suspend(struct device *dev)
{
	uint8_t buf[4] = {0};
	uint32_t i = 0;
	int ret = 0;
	if (!bTouchIsAwake) {
		NVT_LOG("Touch is already suspend\n");
		return 0;
	}
	pm_stay_awake(dev);
	ts->ic_state = NVT_IC_SUSPEND_IN;

	NVT_LOG("cancel delayed work sync\n");
	cancel_delayed_work_sync(&nvt_esd_check_work);
	nvt_esd_check_enable(false);

	NVT_LOG("start\n");

	nvt_irq_enable(false);	/*must before hold lock*/

	mutex_lock(&ts->lock);
	bTouchIsAwake = 0;

	mdelay(10);
	/* ---write command to enter "deep sleep mode"--- */
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x11;
	CTP_SPI_WRITE(ts->client, buf, 2);

	nvt_set_page(0x11a50);
	buf[0] = 0x11a50 & 0xff;
	buf[1] = 0x11;
	CTP_SPI_WRITE(ts->client, buf, 2);
	if (ts->ts_pinctrl) {
		ret = pinctrl_select_state(ts->ts_pinctrl, ts->pinctrl_state_suspend);

		if (ret < 0) {
			NVT_ERR("Failed to select %s pinstate %d\n",
				PINCTRL_STATE_SUSPEND, ret);
		}
	} else {
		NVT_ERR("Failed to init pinctrl\n");
	}
	mdelay(10);
	mutex_unlock(&ts->lock);
	/* release all touches */
	for (i = 0; i < ts->max_touch_num; i++) {
		input_mt_slot(ts->input_dev, i);
		/*input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);*/
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
	}
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
	input_report_key(ts->input_dev, BTN_TOUCH, 0);

	input_sync(ts->input_dev);

	msleep(50);
	if (likely(ts->ic_state == NVT_IC_SUSPEND_IN))
		ts->ic_state = NVT_IC_SUSPEND_OUT;
	else
		NVT_ERR("IC state may error,caused by suspend/resume flow, please CHECK!!");
	NVT_LOG("end\n");
	pm_relax(dev);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen driver resume function.
return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_resume(struct device *dev)
{
	NVT_LOG("nvt_ts_resume\n");
	NVT_LOG("nvt_ts_resume\n");

	int ret;

	if (ts->dev_pm_suspend)
		pm_stay_awake(dev);

	if (ts->ts_pinctrl) {
		ret = pinctrl_select_state(ts->ts_pinctrl, ts->pinctrl_state_active);

		if (ret < 0) {
			NVT_ERR("Failed to select %s pinstate %d\n",
				PINCTRL_STATE_ACTIVE, ret);
		}
	} else {
		NVT_ERR("Failed to init pinctrl\n");
	}
	if (bTouchIsAwake) {
		NVT_LOG("Touch is already resume\n");
		mutex_lock(&ts->lock);
		nvt_update_firmware(ts->firmware_name);
		mutex_unlock(&ts->lock);
		goto Exit;
	}

	ts->ic_state = NVT_IC_RESUME_IN;

	mutex_lock(&ts->lock);
	NVT_LOG("start\n");

	/* please make sure display reset(RESX) sequence and mipi dsi cmds sent before this */
	gpio_set_value(ts->reset_gpio, 1);
	ret = nvt_update_firmware(ts->firmware_name);
	if (ret)
		NVT_ERR("download firmware failed\n");
	nvt_check_fw_reset_state(RESET_STATE_REK);

	nvt_irq_enable(true);

	nvt_esd_check_enable(false);
	queue_delayed_work(nvt_esd_check_wq, &nvt_esd_check_work,
			msecs_to_jiffies(125));

	bTouchIsAwake = 1;
	mutex_unlock(&ts->lock);

	if (likely(ts->ic_state == NVT_IC_RESUME_IN))
		ts->ic_state = NVT_IC_RESUME_OUT;
	else
		NVT_ERR("IC state may error,caused by suspend/resume flow, please CHECK!!");

Exit:
	if (ts->dev_pm_suspend)
		pm_relax(dev);
	NVT_LOG("end\n");

	return 0;
}

static int nvt_pm_suspend(struct device *dev)
{
	ts->dev_pm_suspend = true;
	reinit_completion(&ts->dev_pm_suspend_completion);

	return 0;
}

static int nvt_pm_resume(struct device *dev)
{
	ts->dev_pm_suspend = false;
	complete(&ts->dev_pm_suspend_completion);

	return 0;
}

static const struct dev_pm_ops nvt_dev_pm_ops = {
	.suspend = nvt_pm_suspend,
	.resume = nvt_pm_resume,
};

static const struct spi_device_id nvt_ts_id[] = {
	{ NVT_SPI_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, nvt_ts_id);

#ifdef CONFIG_OF
static struct of_device_id nvt_match_table[] = {
	{ .compatible = "novatek,NVT-ts-spi",},
	{ },
};
MODULE_DEVICE_TABLE(of, nvt_match_table);
#endif

static struct spi_driver nvt_driver = {
	.probe		= nvt_ts_probe,
	.remove		= nvt_ts_remove,
	.shutdown	= nvt_ts_shutdown,
	.id_table	= nvt_ts_id,
	.driver = {
		.name	= NVT_SPI_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &nvt_dev_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table = nvt_match_table,
#endif
	},
};

static int panel_prepared(struct drm_panel_follower *follower)
{
	struct nvt_ts_data *ts = container_of(follower, struct nvt_ts_data, panel_follower);
	NVT_LOG("RESUME_NOTIFIER\n");
        flush_workqueue(ts->event_wq);
        queue_work(ts->event_wq, &ts->resume_work);
	return 0;
}

static int panel_unpreparing(struct drm_panel_follower *follower)
{
	struct nvt_ts_data *ts = container_of(follower, struct nvt_ts_data, panel_follower);
	NVT_LOG("SUSPEND_NOTIFIER\n");
	flush_workqueue(ts->event_wq);
	return nvt_ts_suspend(&ts->client->dev);
}

static struct drm_panel_follower_funcs nvt_ts_follower_funcs = {
	.panel_prepared = panel_prepared,
	.panel_unpreparing = panel_unpreparing,
};

module_spi_driver(nvt_driver);

MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");

