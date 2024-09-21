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

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct ss_ea8076_global {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
};

static inline
struct ss_ea8076_global *to_ss_ea8076_global(struct drm_panel *panel)
{
	return container_of(panel, struct ss_ea8076_global, panel);
}

static void ss_ea8076_global_reset(struct ss_ea8076_global *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int ss_ea8076_global_on(struct ss_ea8076_global *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0xb7, 0x01, 0x4b);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_page_address(dsi, 0x0000, 0x0923);
	if (ret < 0) {
		dev_err(dev, "Failed to set page address: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x23);
	mipi_dsi_dcs_write_seq(dsi, 0xd1, 0x0f);
	mipi_dsi_dcs_write_seq(dsi, 0xe9,
			       0x11, 0x55, 0xa6, 0x75, 0xa3, 0xb9, 0xa1, 0x4a,
			       0x00, 0x1a, 0xb8);
	mipi_dsi_dcs_write_seq(dsi, 0xe1, 0x00, 0x00, 0x02, 0x02, 0x42, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xe2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x0c);
	mipi_dsi_dcs_write_seq(dsi, 0xe1, 0x19);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);

	ret = mipi_dsi_dcs_set_display_brightness(dsi, 0x0000);
	if (ret < 0) {
		dev_err(dev, "Failed to set display brightness: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	msleep(67);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int ss_ea8076_global_off(struct ss_ea8076_global *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	return 0;
}

static int ss_ea8076_global_prepare(struct drm_panel *panel)
{
	struct ss_ea8076_global *ctx = to_ss_ea8076_global(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ss_ea8076_global_reset(ctx);

	ret = ss_ea8076_global_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	return 0;
}

static int ss_ea8076_global_unprepare(struct drm_panel *panel)
{
	struct ss_ea8076_global *ctx = to_ss_ea8076_global(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = ss_ea8076_global_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static const struct drm_display_mode ss_ea8076_global_mode = {
	.clock = (1080 + 22 + 16 + 16) * (2340 + 16 + 16 + 22) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 22,
	.hsync_end = 1080 + 22 + 16,
	.htotal = 1080 + 22 + 16 + 16,
	.vdisplay = 2340,
	.vsync_start = 2340 + 16,
	.vsync_end = 2340 + 16 + 16,
	.vtotal = 2340 + 16 + 16 + 22,
	.width_mm = 68,
	.height_mm = 147,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int ss_ea8076_global_get_modes(struct drm_panel *panel,
				      struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &ss_ea8076_global_mode);
}

static const struct drm_panel_funcs ss_ea8076_global_panel_funcs = {
	.prepare = ss_ea8076_global_prepare,
	.unprepare = ss_ea8076_global_unprepare,
	.get_modes = ss_ea8076_global_get_modes,
};

static int ss_ea8076_global_bl_update_status(struct backlight_device *bl)
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
static int ss_ea8076_global_bl_get_brightness(struct backlight_device *bl)
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

static const struct backlight_ops ss_ea8076_global_bl_ops = {
	.update_status = ss_ea8076_global_bl_update_status,
	.get_brightness = ss_ea8076_global_bl_get_brightness,
};

static struct backlight_device *
ss_ea8076_global_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 2047,
		.max_brightness = 2047,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &ss_ea8076_global_bl_ops, &props);
}

static int ss_ea8076_global_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct ss_ea8076_global *ctx;
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
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &ss_ea8076_global_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = ss_ea8076_global_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void ss_ea8076_global_remove(struct mipi_dsi_device *dsi)
{
	struct ss_ea8076_global *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id ss_ea8076_global_of_match[] = {
	{ .compatible = "samsung,ea8076-global" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ss_ea8076_global_of_match);

static struct mipi_dsi_driver ss_ea8076_global_driver = {
	.probe = ss_ea8076_global_probe,
	.remove = ss_ea8076_global_remove,
	.driver = {
		.name = "panel-ss-ea8076-global",
		.of_match_table = ss_ea8076_global_of_match,
	},
};
module_mipi_dsi_driver(ss_ea8076_global_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for samsung ea8076 fhd cmd dsi panel");
MODULE_LICENSE("GPL");
