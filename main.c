#include <stdio.h>
#include <wayland-client.h>

void registry_global_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    printf("interface: '%s', version: %u, name: %u\n", interface, version, name);
}

void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {
    printf("removed: %u\n", name);
}

int main(void)
{
    struct wl_display *display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    struct wl_registry_listener registry_listener = {
        .global = registry_global_handler,
        .global_remove = registry_global_remove_handler
    };
    wl_registry_add_listener(registry, &registry_listener, NULL);

    while (1) {
        wl_display_dispatch(display);
    }
}
