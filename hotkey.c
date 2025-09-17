#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>


#include <signal.h>
#include <errno.h>

// Default values
#define DEFAULT_INPUT_DEVICE "/dev/input/event1" // Path to the input device to monitor
#define DEFAULT_TABLET_MODE_KEY KEY_POWER  // Key code that triggers the tablet mode switch

// Helper to emit an input event
void emit_event(int fd, __u16 type, __u16 code, int value) {
    struct input_event e = {0};
    e.type = type;
    e.code = code;
    e.value = value;
    if (write(fd, &e, sizeof(e)) < 0) {
        perror("Failed to write event");
    }
}

int main(int argc, char *argv[])
{
    int uinput_fd, event_fd, ret;
    struct uinput_user_dev uidev;
    struct input_event ev;
    int tablet_mode_state = 0;
    const char *input_device = DEFAULT_INPUT_DEVICE;
    int tablet_mode_key = DEFAULT_TABLET_MODE_KEY;

    // Parse command-line arguments
    if (argc > 1) {
        input_device = argv[1];
    }
    if (argc > 2) {
        tablet_mode_key = atoi(argv[2]);
    }

    // Open the uinput device for writing, non-blocking mode
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0)
    {
        perror("Failed to open /dev/uinput");
        return -1;
    }

    // Initialize the virtual device
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Fake Intel HID switches");
    uidev.id.bustype = BUS_HOST;
    uidev.id.vendor = 0x1234;
    uidev.id.product = 0x1234;
    uidev.id.version = 0x1234;

    // Enable switch events and the tablet mode switch
    ioctl(uinput_fd, UI_SET_EVBIT, EV_SW);
    ioctl(uinput_fd, UI_SET_SWBIT, SW_TABLET_MODE);

    // Create the virtual device
    write(uinput_fd, &uidev, sizeof(uidev));
    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0)
    {
        perror("Failed to create uinput device");
        close(uinput_fd);
        return -1;
    }

    // Open the input device for reading
    event_fd = open(input_device, O_RDONLY | O_NONBLOCK);
    if (event_fd < 0)
    {
        perror("Failed to open device:");
        perror(INPUT_DEVICE);
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        return -1;
    }

    // Main loop: wait for and process events from the input device
    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(event_fd, &fds);

    // Wait for an event using select()
        ret = select(event_fd + 1, &fds, NULL, NULL, NULL);
        if (ret == -1) {
            if (errno == EINTR) {
                // Interruption par signal, sortir proprement
                break;
            } else {
                perror("select error");
                break;
            }
        }
        if (ret > 0 && FD_ISSET(event_fd, &fds))
        {
            // Read the event
            ret = read(event_fd, &ev, sizeof(struct input_event));
            if (ret == sizeof(struct input_event))
            {
                if (ev.type == EV_KEY && ev.code == tablet_mode_key && ev.value == 1)
                {
                    // Toggle tablet mode state
                    tablet_mode_state = !tablet_mode_state;

                    emit_event(uinput_fd, EV_SW, SW_TABLET_MODE, tablet_mode_state);
                    emit_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
                }
            }
        } else {
            // Sleep to save CPU if no event
            usleep(10000); // 10 ms
        }
    }

    // Release resources
    close(event_fd);
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);

    return 0;
}
