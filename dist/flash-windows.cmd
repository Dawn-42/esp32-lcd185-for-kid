@echo off
setlocal
cd /d "%~dp0"
for %%F in (YouyouSpace.bootloader.bin YouyouSpace.partitions.bin boot_app0.bin YouyouSpace.app.bin) do (
  if not exist "%%F" (
    echo Missing %%F. Extract the complete firmware package first.
    pause
    exit /b 1
  )
)
py -3 --version >nul 2>nul
if errorlevel 1 (
  echo Install Python 3 with the Windows Python Launcher from python.org first.
  pause
  exit /b 1
)
py -3 -c "import esptool,sys; sys.exit(0 if esptool.__version__ == '4.8.1' else 1)" >nul 2>nul
if errorlevel 1 (
  py -3 -m pip install --disable-pip-version-check esptool==4.8.1
  if errorlevel 1 goto failed
)
py -3 -c "import esptool,sys; sys.exit(0 if esptool.__version__ == '4.8.1' else 1)" >nul 2>nul
if errorlevel 1 goto failed
echo Target: Waveshare ESP32-S3-Touch-LCD-1.85B, 16MB Flash, OPI PSRAM.
echo Connect with a USB DATA cable and close all serial monitors.
echo Find the COM port in Windows Device Manager. BOOT mode may use a new port.
set "YOUYOU_COM="
set "YOUYOU_PORT="
set /p "YOUYOU_COM=Enter COM port, for example COM5: "
for /f "delims=" %%P in ('py -3 -c "import os,re; p=os.environ.get('YOUYOU_COM','').strip().upper(); print(p if re.fullmatch('COM[1-9][0-9]*',p) else '')"') do set "YOUYOU_PORT=%%P"
if not defined YOUYOU_PORT (
  echo Invalid COM port. Use COM followed by a positive integer, for example COM5.
  goto failed
)
py -3 -m esptool --chip esp32s3 --port "%YOUYOU_PORT%" --baud 115200 write_flash 0x0 "YouyouSpace.bootloader.bin" 0x8000 "YouyouSpace.partitions.bin" 0xe000 "boot_app0.bin" 0x10000 "YouyouSpace.app.bin"
if errorlevel 1 goto failed
echo Flash write and verification completed. Application startup is not checked here.
echo If BOOT download mode was used, release BOOT and fully power-cycle the board.
echo With a battery attached, unplugging USB alone may NOT fully power it off.
echo See README for the USB and battery restart steps. Do not erase all Flash.
pause
exit /b 0
:failed
echo Flash did not complete. Check Python, network, COM port, cable and BOOT mode.
echo See README troubleshooting. No application-startup success is implied.
pause
exit /b 1
