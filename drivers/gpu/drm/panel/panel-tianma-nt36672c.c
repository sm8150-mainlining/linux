// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct j20s_36_02_0a_dsc {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct regulator *supply;
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline
struct j20s_36_02_0a_dsc *to_j20s_36_02_0a_dsc(struct drm_panel *panel)
{
	return container_of(panel, struct j20s_36_02_0a_dsc, panel);
}

static void j20s_36_02_0a_dsc_reset(struct j20s_36_02_0a_dsc *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int j20s_36_02_0a_dsc_on(struct j20s_36_02_0a_dsc *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;
	
	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x24);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x4d, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0x4e, 0x30);
	mipi_dsi_dcs_write_seq(dsi, 0x4f, 0x30);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x30);
	mipi_dsi_dcs_write_seq(dsi, 0x7a, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x7b, 0x8c);
	mipi_dsi_dcs_write_seq(dsi, 0x7d, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0x80, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0x81, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xa0, 0x0c);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_READ_PPS_START, 0x0c);
	mipi_dsi_dcs_write_seq(dsi, 0xa3, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xa4, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xa5, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xc4, 0x80);
	mipi_dsi_dcs_write_seq(dsi, 0xc6, 0xc0);
	mipi_dsi_dcs_write_seq(dsi, 0xe9, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x25);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xda, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xe0, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xf1, 0x04);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x2b);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xb7, 0x08);
	mipi_dsi_dcs_write_seq(dsi, 0xb8, 0x1a);
	mipi_dsi_dcs_write_seq(dsi, 0xc0, 0x04);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0xf0);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x1c, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x33, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x23);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x00, 0x80);
	mipi_dsi_dcs_write_seq(dsi, 0x01, 0x84);
	mipi_dsi_dcs_write_seq(dsi, 0x05, 0x2d);
	mipi_dsi_dcs_write_seq(dsi, 0x06, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x07, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x08, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x09, 0x45);
	mipi_dsi_dcs_write_seq(dsi, 0x11, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x12, 0x95);
	mipi_dsi_dcs_write_seq(dsi, 0x15, 0x68);
	mipi_dsi_dcs_write_seq(dsi, 0x16, 0x0b);
	mipi_dsi_dcs_write_seq(dsi, 0x29, 0x0a);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_PARTIAL_ROWS, 0xff);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_PARTIAL_COLUMNS, 0xfe);
	mipi_dsi_dcs_write_seq(dsi, 0x32, 0xfd);
	mipi_dsi_dcs_write_seq(dsi, 0x33, 0xfb);
	mipi_dsi_dcs_write_seq(dsi, 0x34, 0xf8);
	mipi_dsi_dcs_write_seq(dsi, 0x35, 0xf5);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_ADDRESS_MODE, 0xf3);
	mipi_dsi_dcs_write_seq(dsi, 0x37, 0xf2);
	mipi_dsi_dcs_write_seq(dsi, 0x38, 0xf2);
	mipi_dsi_dcs_write_seq(dsi, 0x39, 0xf2);

	ret = mipi_dsi_dcs_set_pixel_format(dsi, 0xef);
	if (ret < 0) {
		dev_err(dev, "Failed to set pixel format: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0x3b, 0xec);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_3D_CONTROL, 0xe9);
	mipi_dsi_dcs_write_seq(dsi, 0x3f, 0xe5);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_VSYNC_TIMING, 0xe5);
	mipi_dsi_dcs_write_seq(dsi, 0x41, 0xe5);
	mipi_dsi_dcs_write_seq(dsi, 0x2a, 0x13);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_GET_SCANLINE, 0xff);
	mipi_dsi_dcs_write_seq(dsi, 0x46, 0xf4);
	mipi_dsi_dcs_write_seq(dsi, 0x47, 0xe7);
	mipi_dsi_dcs_write_seq(dsi, 0x48, 0xda);
	mipi_dsi_dcs_write_seq(dsi, 0x49, 0xcd);
	mipi_dsi_dcs_write_seq(dsi, 0x4a, 0xc0);
	mipi_dsi_dcs_write_seq(dsi, 0x4b, 0xb3);
	mipi_dsi_dcs_write_seq(dsi, 0x4c, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0x4d, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0x4e, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0x4f, 0x99);
	mipi_dsi_dcs_write_seq(dsi, 0x50, 0x80);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x0068);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0x52, 0x66);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x66);
	mipi_dsi_dcs_write_seq(dsi, 0x54, 0x66);
	mipi_dsi_dcs_write_seq(dsi, 0x2b, 0x0e);
	mipi_dsi_dcs_write_seq(dsi, 0x58, 0xff);
	mipi_dsi_dcs_write_seq(dsi, 0x59, 0xfb);
	mipi_dsi_dcs_write_seq(dsi, 0x5a, 0xf7);
	mipi_dsi_dcs_write_seq(dsi, 0x5b, 0xf3);
	mipi_dsi_dcs_write_seq(dsi, 0x5c, 0xef);
	mipi_dsi_dcs_write_seq(dsi, 0x5d, 0xe3);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, 0xda);
	mipi_dsi_dcs_write_seq(dsi, 0x5f, 0xd8);
	mipi_dsi_dcs_write_seq(dsi, 0x60, 0xd8);
	mipi_dsi_dcs_write_seq(dsi, 0x61, 0xd8);
	mipi_dsi_dcs_write_seq(dsi, 0x62, 0xcb);
	mipi_dsi_dcs_write_seq(dsi, 0x63, 0xbf);
	mipi_dsi_dcs_write_seq(dsi, 0x64, 0xb3);
	mipi_dsi_dcs_write_seq(dsi, 0x65, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0x66, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0x67, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x27);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_VSYNC_TIMING, 0x20);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x10);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xc0, 0x03);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0xb50d);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x24);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x10);
	mipi_dsi_dcs_write_seq(dsi, 0x11, 0x00);
	
	msleep(70);
	
	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}
	
	msleep(40);
	
	mipi_dsi_dcs_write_seq(dsi, 0x29, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x27);
	mipi_dsi_dcs_write_seq(dsi, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x3f, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0x43, 0x08);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_VSYNC_TIMING, 0x25);
	mipi_dsi_dcs_write_seq(dsi, 0xff, 0x10);

	return 0;
}

static int j20s_36_02_0a_dsc_off(struct j20s_36_02_0a_dsc *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	
	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	return 0;
}

static int j20s_36_02_0a_dsc_prepare(struct drm_panel *panel)
{
	struct j20s_36_02_0a_dsc *ctx = to_j20s_36_02_0a_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulator: %d\n", ret);
		return ret;
	}

	j20s_36_02_0a_dsc_reset(ctx);

	ret = j20s_36_02_0a_dsc_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_disable(ctx->supply);
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

	ctx->prepared = true;
	return 0;
}

static int j20s_36_02_0a_dsc_unprepare(struct drm_panel *panel)
{
	struct j20s_36_02_0a_dsc *ctx = to_j20s_36_02_0a_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = j20s_36_02_0a_dsc_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_disable(ctx->supply);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode j20s_36_02_0a_dsc_mode = {
	.clock = (1080 + 67 + 12 + 40) * (2400 + 33 + 2 + 30) * 120 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 67,
	.hsync_end = 1080 + 67 + 12,
	.htotal = 1080 + 67 + 12 + 40,
	.vdisplay = 2400,
	.vsync_start = 2400 + 33,
	.vsync_end = 2400 + 33 + 2,
	.vtotal = 2400 + 33 + 2 + 30,
	.width_mm = 70,
	.height_mm = 154,
};

static int j20s_36_02_0a_dsc_get_modes(struct drm_panel *panel,
				       struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &j20s_36_02_0a_dsc_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs j20s_36_02_0a_dsc_panel_funcs = {
	.prepare = j20s_36_02_0a_dsc_prepare,
	.unprepare = j20s_36_02_0a_dsc_unprepare,
	.get_modes = j20s_36_02_0a_dsc_get_modes,
};

static int j20s_36_02_0a_dsc_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct j20s_36_02_0a_dsc *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supply = devm_regulator_get(dev, "vddio");
	if (IS_ERR(ctx->supply))
		return dev_err_probe(dev, PTR_ERR(ctx->supply),
				     "Failed to get vddio regulator\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &j20s_36_02_0a_dsc_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get backlight\n");

	drm_panel_add(&ctx->panel);

	/* This panel only supports DSC; unconditionally enable it */
	dsi->dsc = &ctx->dsc;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;

	/* TODO: Pass slice_per_pkt = 2 */
	ctx->dsc.slice_height = 20;
	ctx->dsc.slice_width = 540;
	/*
	 * TODO: hdisplay should be read from the selected mode once
	 * it is passed back to drm_panel (in prepare?)
	 */
	WARN_ON(1080 % ctx->dsc.slice_width);
	ctx->dsc.slice_count = 1080 / ctx->dsc.slice_width;
	ctx->dsc.bits_per_component = 8;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 fractional bits */
	ctx->dsc.block_pred_enable = true;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static void j20s_36_02_0a_dsc_remove(struct mipi_dsi_device *dsi)
{
	struct j20s_36_02_0a_dsc *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id j20s_36_02_0a_dsc_of_match[] = {
	{ .compatible = "tianma,nt36672c" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, j20s_36_02_0a_dsc_of_match);

static struct mipi_dsi_driver j20s_36_02_0a_dsc_driver = {
	.probe = j20s_36_02_0a_dsc_probe,
	.remove = j20s_36_02_0a_dsc_remove,
	.driver = {
		.name = "panel-j20s-36-02-0a-dsc",
		.of_match_table = j20s_36_02_0a_dsc_of_match,
	},
};
module_mipi_dsi_driver(j20s_36_02_0a_dsc_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for xiaomi 36 02 0a video mode dsc dsi panel");
MODULE_LICENSE("GPL");
