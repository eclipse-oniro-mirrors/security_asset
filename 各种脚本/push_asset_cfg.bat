hdc shell mount -o rw,remount /
hdc file send huks_service.json /system/profile/huks_service.json
hdc shell reboot
pause