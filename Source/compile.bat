gcc RemoteScreenshot.c -o RemoteScreenshot.exe -lnetapi32 -ladvapi32
gcc RemoteScreenshotSvc.c -o RemoteScreenshotSvc.exe -lgdi32

gcc Screenshot.c -o Screenshot.exe -lgdi32
