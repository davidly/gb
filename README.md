# gb
Get Bitmap. Gets a screen capture of an app and saves it to an image file

Build using a Microsoft Visual Studio 64 bit command prompt using m.bat

    usage: gb [-v] [-w] [appname] [outputfile]
    Get Bitmap: creates a bitmap containing contents of a window
    arguments: [appname]       Name of the app to capture. Can contain wildcards.
               [outputfile]    Name of the image file to create.
               [-v]            Verbose logging of debugging information.
               [-w]            Capture the whole window, not just the client area.
    sample usage: (arguments can use - or /)
      gb inbox*outlook outlook.png  Saves a snapshot of the window in the PNG file
      gb *excel excel.jpg           Saves a snapshot of the window to the JPG file
      gb -w calc* calc.jpg          Saves a snapshot of the whole window to the JPG file
      gb 0x6bf0 calc.bmp            Saves a snapshot of the window with this PID to the BMP file
      gb                            Shows titles and positions of visible top-level windows
    notes:
      - if no arguments are specified then all window positions are printed
      - appname can contain ? and * characters as wildcards
      - appname is case insensitive
      - appname can alternatively be a window handle or process id in decimal or hex
      - only visible, top-level windows are considered
      - the screen is grabbed, so obscurring windows are shown
      - if no image type can be inferred from the extension, JPG is assumed
      - extra pixels surrounding windows exist because Windows draws narrow inner borders
        and transparent outer edges for borders that are in fact wide
      - 'modern' app client area includes the chrome
      
  sample output with no arguments:
  
       left    top  right bottom       hwnd   pid text
      -2899     64   -904   2116   0x631458 15056 'Editing gb/README.md at main ╖ davidly/gb - Personal - Microsoft  Edge'
      -3840      0  -1845   2160    0x4008c 15056 'PV/main.yml at main ╖ davidly/PV and 5 more pages - Personal - Microsoft  Edge'
       1683    493   3632   2058   0x141b0a 16840 'Administrator: cmd.exe'
       1958   1140   3323   2045  0x31608e2 24124 'Calculator'
        624    188   2063   2006    0x309c6 11816 'Administrator: cmd.exe'
        108    172   1677   1898   0xe00e68  4272 'Administrator: cmd.exe'
        273    582   1488   1622    0xe1406 11532 'Downloads'
        252    377   1381   1942    0x70e48  5184 'Administrator: cmd.exe'
      -1840      6      4   2081    0xf0a2c 27632 'Upcoming shows  - OneNote'
          0      1   1500   1166  0x167088a 29832 'OneNote for Windows 10'
         50     40   1568   1215   0x750d28 24124 'OneNote for Windows 10'
          0      1   1465   1333  0x1830898  6932 'Settings'
      -1842    354   -359   1696   0x2406fc 24124 'Settings'
       3664     45   3840     90    0x70940  2788 'Seconds'
       3540      0   3840     45    0x1063c 21732 'CPU'
          0      0   3840   2160    0x10386  8104 'Windows Input Experience'
          0      0   3840   2160    0x10308 15576 'NVIDIA GeForce Overlay'
      -3840      0   3840   2160    0x1015a 11532 'Program Manager'
      
