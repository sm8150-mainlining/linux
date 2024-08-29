// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct samsung_oneplus_dsc {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
};

static inline
struct samsung_oneplus_dsc *to_samsung_oneplus_dsc(struct drm_panel *panel)
{
	return container_of(panel, struct samsung_oneplus_dsc, panel);
}

static void samsung_oneplus_dsc_reset(struct samsung_oneplus_dsc *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(8000, 9000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
}

static int samsung_oneplus_dsc_on(struct samsung_oneplus_dsc *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_compression_mode(dsi, true);
	if (ret < 0) {
		dev_err(dev, "Failed to set compression mode: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0xc3, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x0a,
			       0x10, 0x00, 0x00, 0x89, 0x30, 0x80, 0x0c, 0x30,
			       0x05, 0xa0, 0x00, 0x41, 0x02, 0xd0, 0x02, 0xd0,
			       0x02, 0x00, 0x02, 0xc2, 0x00, 0x20, 0x06, 0x58,
			       0x00, 0x0a, 0x00, 0x0f, 0x01, 0xe0, 0x01, 0x2d,
			       0x18, 0x00, 0x10, 0xf0, 0x03, 0x0c, 0x20, 0x00,
			       0x06, 0x0b, 0x0b, 0x33, 0x0e, 0x1c, 0x2a, 0x38,
			       0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b,
			       0x7d, 0x7e, 0x01, 0x02, 0x01, 0x00, 0x09, 0x40,
			       0x09, 0xbe, 0x19, 0xfc, 0x19, 0xfa, 0x19, 0xf8,
			       0x1a, 0x38, 0x1a, 0x78, 0x1a, 0xb6, 0x2a, 0xb6,
			       0x2a, 0xf4, 0x2a, 0xf4, 0x4b, 0x34, 0x63, 0x74,
			       0x00);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_set_column_address(dsi, 0x0000, 0x059f);
	if (ret < 0) {
		dev_err(dev, "Failed to set column address: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_set_page_address(dsi, 0x0000, 0x0c2f);
	if (ret < 0) {
		dev_err(dev, "Failed to set page address: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb9,
			       0x01, 0xc0, 0x3c, 0x0b, 0x00, 0x00, 0x00, 0x11,
			       0x03);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x45);
	mipi_dsi_dcs_write_seq(dsi, 0xb5, 0x48);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x1e);
	mipi_dsi_dcs_write_seq(dsi, 0xc5, 0x09, 0x10, 0xb4, 0x24, 0xfb);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x37);
	mipi_dsi_dcs_write_seq(dsi, 0xc5, 0x04, 0xff, 0x00, 0x01, 0x64);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x86);
	mipi_dsi_dcs_write_seq(dsi, 0xeb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xb1, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xb5, 0xd3);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int samsung_oneplus_dsc_off(struct samsung_oneplus_dsc *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	return 0;
}

static int samsung_oneplus_dsc_prepare(struct drm_panel *panel)
{
	struct samsung_oneplus_dsc *ctx = to_samsung_oneplus_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	samsung_oneplus_dsc_reset(ctx);

	ret = samsung_oneplus_dsc_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);

	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret < 0) {
		dev_err(panel->dev, "failed to transmit PPS: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_compression_mode(ctx->dsi, true);
	if (ret < 0) {
		dev_err(dev, "failed to enable compression mode: %d\n", ret);
		return ret;
	}

	msleep(28); /* TODO: Is this panel-dependent? */

	return 0;
}

static int samsung_oneplus_dsc_unprepare(struct drm_panel *panel)
{
	struct samsung_oneplus_dsc *ctx = to_samsung_oneplus_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = samsung_oneplus_dsc_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static const struct drm_display_mode samsung_oneplus_dsc_mode = {
	.clock = (1440 + 16 + 8 + 8) * (3120 + 400 + 28 + 1156) * 60 / 1000,
	.hdisplay = 1440,
	.hsync_start = 1440 + 16,
	.hsync_end = 1440 + 16 + 8,
	.htotal = 1440 + 16 + 8 + 8,
	.vdisplay = 3120,
	.vsync_start = 3120 + 400,
	.vsync_end = 3120 + 400 + 28,
	.vtotal = 3120 + 400 + 28 + 1156,
	.width_mm = 71,
	.height_mm = 154,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int samsung_oneplus_dsc_get_modes(struct drm_panel *panel,
					 struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &samsung_oneplus_dsc_mode);
}

static const struct drm_panel_funcs samsung_oneplus_dsc_panel_funcs = {
	.prepare = samsung_oneplus_dsc_prepare,
	.unprepare = samsung_oneplus_dsc_unprepare,
	.get_modes = samsung_oneplus_dsc_get_modes,
};

static int samsung_oneplus_dsc_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int samsung_oneplus_dsc_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness_large(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops samsung_oneplus_dsc_bl_ops = {
	.update_status = samsung_oneplus_dsc_bl_update_status,
	.get_brightness = samsung_oneplus_dsc_bl_get_brightness,
};

static struct backlight_device *
samsung_oneplus_dsc_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 1023,
		.max_brightness = 1023,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &samsung_oneplus_dsc_bl_ops, &props);
}

static int samsung_oneplus_dsc_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct samsung_oneplus_dsc *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &samsung_oneplus_dsc_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = samsung_oneplus_dsc_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	/* This panel only supports DSC; unconditionally enable it */
	dsi->dsc = &ctx->dsc;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;

	/* TODO: Pass slice_per_pkt = 2 */
	ctx->dsc.slice_height = 65;
	ctx->dsc.slice_width = 720;
	/*
	 * TODO: hdisplay should be read from the selected mode once
	 * it is passed back to drm_panel (in prepare?)
	 */
	WARN_ON(1440 % ctx->dsc.slice_width);
	ctx->dsc.slice_count = 1440 / ctx->dsc.slice_width;
	ctx->dsc.bits_per_component = 8;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 fractional bits */
	ctx->dsc.block_pred_enable = true;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void samsung_oneplus_dsc_remove(struct mipi_dsi_device *dsi)
{
	struct samsung_oneplus_dsc *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id samsung_oneplus_dsc_of_match[] = {
	{ .compatible = "samsung,oneplus-dsc" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, samsung_oneplus_dsc_of_match);

static struct mipi_dsi_driver samsung_oneplus_dsc_driver = {
	.probe = samsung_oneplus_dsc_probe,
	.remove = samsung_oneplus_dsc_remove,
	.driver = {
		.name = "panel-samsung-oneplus-dsc",
		.of_match_table = samsung_oneplus_dsc_of_match,
	},
};
module_mipi_dsi_driver(samsung_oneplus_dsc_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for samsung dsc cmd mode oneplus dsi panel");
MODULE_LICENSE("GPL");
