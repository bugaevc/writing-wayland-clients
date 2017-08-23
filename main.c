#include <stdio.h>
#include <string.h>

#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include "xdg-shell.h"

struct wl_compositor *compositor;
struct wl_shm *shm;
struct wl_seat *seat;
struct zxdg_shell_v6 *xdg_shell;

struct wl_pointer *pointer;

struct wl_surface *cursor_surface;
struct wl_cursor_image *cursor_image;

void registry_global_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry, name,
            &wl_compositor_interface, 3);
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(registry, name,
            &wl_shm_interface, 1);
    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(registry, name,
            &wl_seat_interface, 1);
    } else if (strcmp(interface, "zxdg_shell_v6") == 0) {
        xdg_shell = wl_registry_bind(registry, name,
            &zxdg_shell_v6_interface, 1);
    }
}

void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {}

const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler
};

void xdg_toplevel_configure_handler
(
    void *data,
    struct zxdg_toplevel_v6 *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states
) {
    printf("configure: %dx%d\n", width, height);
}

void xdg_toplevel_close_handler
(
    void *data,
    struct zxdg_toplevel_v6 *xdg_toplevel
) {
    printf("close\n");
}

const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler
};

void xdg_surface_configure_handler
(
    void *data,
    struct zxdg_surface_v6 *xdg_surface,
    uint32_t serial
) {
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
}

const struct zxdg_surface_v6_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler
};

void xdg_shell_ping_handler
(
    void *data,
    struct zxdg_shell_v6 *xdg_shell,
    uint32_t serial
) {
    zxdg_shell_v6_pong(xdg_shell, serial);
    printf("ping-pong\n");
}

const struct zxdg_shell_v6_listener xdg_shell_listener = {
    .ping = xdg_shell_ping_handler
};

void pointer_enter_handler
(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t x,
    wl_fixed_t y
)
{
    wl_pointer_set_cursor(pointer, serial, cursor_surface,
        cursor_image->hotspot_x, cursor_image->hotspot_y);

    printf("enter:\t%d %d\n",
        wl_fixed_to_int(x),
        wl_fixed_to_int(y));
}

void pointer_leave_handler
(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface
)
{
    printf("leave\n");
}

void pointer_motion_handler
(
    void *data,
    struct wl_pointer *pointer,
    uint32_t time,
    wl_fixed_t x,
    wl_fixed_t y
)
{
    printf("motion:\t%d %d\n",
        wl_fixed_to_int(x),
        wl_fixed_to_int(y));
}

void pointer_button_handler
(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state
)
{
    printf("button: 0x%x state: %d\n", button, state);
}

void pointer_axis_handler
(
    void *data,
    struct wl_pointer *pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value
)
{
    printf("axis: %d %f\n", axis, wl_fixed_to_double(value));
}

const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter_handler,
    .leave = pointer_leave_handler,
    .motion = pointer_motion_handler,
    .button = pointer_button_handler,
    .axis = pointer_axis_handler
};

int main(void)
{
    struct wl_display *display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);

    // this is only going to work if your computer
    // has a pointing device
    pointer = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(pointer, &pointer_listener, NULL);

    struct wl_cursor_theme *cursor_theme =
        wl_cursor_theme_load(NULL, 24, shm);
    struct wl_cursor *cursor =
        wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
    cursor_image = cursor->images[0];
    struct wl_buffer *cursor_buffer =
        wl_cursor_image_get_buffer(cursor_image);

    cursor_surface = wl_compositor_create_surface(compositor);
    wl_surface_attach(cursor_surface, cursor_buffer, 0, 0);
    wl_surface_commit(cursor_surface);

    zxdg_shell_v6_add_listener(xdg_shell, &xdg_shell_listener, NULL);

    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    struct zxdg_surface_v6 *xdg_surface =
        zxdg_shell_v6_get_xdg_surface(xdg_shell, surface);
    zxdg_surface_v6_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    struct zxdg_toplevel_v6 *xdg_toplevel =
        zxdg_surface_v6_get_toplevel(xdg_surface);
    zxdg_toplevel_v6_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    // signal that the surface is ready to be configured
    wl_surface_commit(surface);

    int width = 200;
    int height = 200;
    int stride = width * 4;
    int size = stride * height;  // bytes

    // open an anonymous file and write some zero bytes to it
    int fd = syscall(SYS_memfd_create, "buffer", 0);
    ftruncate(fd, size);

    // map it to the memory
    unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // turn it into a shared memory pool
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

    // allocate the buffer in that pool
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
        0, width, height, stride, WL_SHM_FORMAT_ARGB8888);

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {

            struct pixel {
                // little-endian ARGB
                unsigned char blue;
                unsigned char green;
                unsigned char red;
                unsigned char alpha;
            } *px = (struct pixel *) (data + y * stride + x * 4);

            // draw a cross
            if ((80 < x && x < 120) || (80 < y && y < 120)) {
                // gradient from blue at the top to white at the bottom
                px->alpha = 255;
                px->red = (double) y / height * 255;
                px->green = px->red;
                px->blue = 255;
            } else {
                // transparent
                px->alpha = 0;
            }
        }
    }

    // wait for the surface to be configured
    wl_display_roundtrip(display);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    while (1) {
        wl_display_dispatch(display);
    }
}
