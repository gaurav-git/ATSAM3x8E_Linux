# ATSAM3x8E_Linux
Arduino Due Firmware Development Env/BSP for Linux Enviroment

References:
http://www.atwillys.de/content/cc/using-custom-ide-and-system-library-on-arduino-due-sam3x8e/?lang=en

Troubleshooting:
If the PHP script just simply prints the script on stdout instead of executing then please edit `/etc/php/5.6/cli/php.ini` and change `short_open_tag = Off` to
`short_open_tag = On`
