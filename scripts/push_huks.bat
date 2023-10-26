hdc shell mount -o rw,remount /vendor
hdc shell mount -o rw,remount /

hdc file send ./49fbc428-af44-11ec-b909-0242ac120002.sec /vendor/bin/
hdc file send huks\libhuks_service.z.so ./system/lib64/
hdc file send huks\libhukssdk.z.so ./system/lib64/
hdc file send huks\libhuks.z.so ./system/lib64/
hdc file send huks\libhuks_ndk.z.so ./system/lib64/

pause
hdc shell reboot
pause