hdc shell mount -o rw,remount /
hdc shell "sed -i 's/enforcing/permissive/g' /system/etc/selinux/config"
hdc shell "cat /system/etc/selinux/config |grep SELINUX="

@echo "Reboot device..."

hdc shell reboot

pause