all: tablet-mode-switch

tablet-mode-switch:
	gcc -o tablet-mode-switch hotkey.c -levdev -ludev -I/usr/include/libevdev-1.0 -Wl,-z,noexecstack

install: tablet-mode-switch
	cp tablet-mode-switch /usr/local/sbin
	cp tablet-mode-switch.service /etc/systemd/system
	systemctl enable --now tablet-mode-switch.service

clean:
	rm tablet-mode-switch

uninstall:
	-systemctl stop tablet-mode-switch.service
	-systemctl disable tablet-mode-switch.service
	-rm /usr/local/sbin/tablet-mode-switch
