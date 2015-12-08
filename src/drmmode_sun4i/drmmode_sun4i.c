/*
 * Copyright © 2013 ARM Limited.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "../drmmode_driver.h"

/* Cursor dimensions
 * Technically we probably don't have any size limit.. since we
 * are just using an overlay... but xserver will always create
 * cursor images in the max size, so don't use width/height values
 * that are too big
 */
/* width */
#define CURSORW   (64)
/* height */
#define CURSORH   (64)
/* Padding added down each side of cursor image */
#define CURSORPAD (0)

/*
 * Attempt to set "zpos" for our cursor plane.
 * This is not strictly needed as the plane is always above the primary
 */
static int init_plane_for_cursor(int drm_fd, uint32_t plane_id)
{
	drmModeObjectPropertiesPtr props;
	int i;

	/* Get properties for our assigned cursor plane */
	props = drmModeObjectGetProperties(drm_fd, plane_id,
					   DRM_MODE_OBJECT_PLANE);
	if (!props)
		return 0;

	/* Find the "zpos" property */
	for (i = 0; i < props->count_props; i++) {
		drmModePropertyPtr prop;

		prop = drmModeGetProperty(drm_fd, props->props[i]);
		if (!prop)
			continue;

		/* zpos is a range property. Set it to the maximum value */
		if (!strncmp(prop->name, "zpos", DRM_PROP_NAME_LEN) &&
		    drm_property_type_is(prop, DRM_MODE_PROP_RANGE) &&
		    prop->count_values == 2) {
			xf86DrvMsg(-1, X_INFO,
				   "Setting zpos for cursor plane %" PRIu32 " to %" PRIu64 "\n",
				   plane_id, prop->values[1]);
			drmModeObjectSetProperty(drm_fd, plane_id,
						 DRM_MODE_OBJECT_PLANE,
						 prop->prop_id,
						 prop->values[1]);
		}

		drmModeFreeProperty(prop);
	}

	drmModeFreeObjectProperties(props);

	return 0;
}

static int create_custom_gem(int fd, struct armsoc_create_gem *create_gem)
{
	struct drm_mode_create_dumb create_arg;
	int ret;

	memset(&create_arg, 0, sizeof(create_arg));
	create_arg.bpp = create_gem->bpp;
	create_arg.width = create_gem->width;
	create_arg.height = create_gem->height;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg);
	if (ret)
		return ret;

	/* Convert create_arg to generic create_gem */
	create_gem->handle = create_arg.handle;
	create_gem->pitch = create_arg.pitch;
	create_gem->size = create_arg.size;

	return 0;
}

struct drmmode_interface sun4i_interface = {
	"sun4i-drm"           /* name of drm driver*/,
	1                     /* use_page_flip_events */,
	1                     /* use_early_display */,
	CURSORW               /* cursor width */,
	CURSORH               /* cursor_height */,
	CURSORPAD             /* cursor padding */,
	HWCURSOR_API_PLANE    /* cursor_api */,
	init_plane_for_cursor /* init_plane_for_cursor */,
	1                     /* vblank_query_supported */,
	create_custom_gem     /* create_custom_gem */,
};
