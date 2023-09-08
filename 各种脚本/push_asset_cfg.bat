hdc shell mount -o rw,remount /
hdc file send huks_service.json /system/profile/huks_service.json
hdc shell mkdir /data/service/el1/public/asset_service
hdc shell chown  huks_server /data/service/el1/public/asset_service
hdc shell reboot
pause