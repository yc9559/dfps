# Dynamic refresh rate configuration help

## Feature options

### touchSlackMs

During operation, the screen refresh rate switches to `activeRefreshRate`, and waits for `touchSlackMs` milliseconds after the operation is released, then switches to `idleRefreshRate`.
This delay is used to avoid frequent switching of the screen refresh rate to reduce the additional power consumption introduced by the switching overhead.
Default value `4000`, min value `100`.

### enableMinBrightness

When the OLED screen switches the refresh rate under low brightness, there may be perceivable changes in brightness and color, resulting in a poor look and feel.
This minimum brightness is used to specify the minimum brightness to enable the dynamic refresh rate, and the brightness value is lower than `enableMinBrightness` to use the `activeRefreshRate`.Default value `8`, max value `255`.  

dfps dynamically obtains the current screen brightness value from the system. Considering that this operation has an overhead of about 100ms, it can be obtained once every 10 seconds at the fastest.
In order to ensure the look and feel, if there is no operation input when using `activeRefreshRate`, the dynamic refresh rate switching will not be restarted if the screen brightness exceeds `enableMinBrightness`.
The brightness value obtained from the system is not proportional to the brightness bar set by the system, nor is it proportional to the actual brightness of the screen. Take MIUI14 screen brightness and brightness value as an example:
```
255 = Auto boost brightness 100%
128 = Manual brightness 100%
64  = Manual brightness 85%
32  = Manual brightness 70%
16  = Manual brightness 50%
8   = Manual brightness 30%
1   = Manual brightness 0%
```

### useSfBackdoor

dfps supports 2 refresh rate switching methods to adapt to more devices.
Use `PEAK_REFRESH_RATE` method if `useSfBackdoor` = 0. Use `Surfaceflinger backdoor` method if `useSfBackdoor` = 1.  
Default value `0`, optional `0` and `1`.  

#### PEAK_REFRESH_RATE

Call Android's native dynamic refresh rate interface, the setting value is intuitive, but it is not suitable for all devices.
The value in the perapp configuration >=20, the value is the refresh rate supported by the system, please do not set the frame rate not supported by the system.

#### Surfaceflinger backdoor

Call Surfaceflinger's debugging interface, the setting value is not intuitive, but it is suitable for most devices.
The value in the perapp configuration is <20, the value is the screen configuration index supported by the system, and the refresh rate corresponding to 0/1/2/... needs to be tried by yourself.

The following is the correspondence between several possible values and the refresh rate for reference:
```
0:120hz, 1:90hz, 2:60hz
0:60hz, 1:90hz
0:30hz, 1:50hz, 2:60hz, 3:90hz, 4:120hz, 5:144hz
0:1080p60hz, 1:1440p120hz, 2:1440p60hz, 3:1080p120hz
```

## 分应用配置

Format: packageName idleValue activeValue. 
The priority of the per-app rules is ordered from high to low. 
"-" = offscreen. "*" = default. Offscreen and default rule must be specified. 
"-1" means use the default screen refresh rate rule of system. 

Example for `PEAK_REFRESH_RATE` method:
```
com.miHoYo.Yuanshen 60 60
com.hypergryph.arknights 60 60
- -1 -1
* 60 120
```

Example for `Surfaceflinger backdoor` method:
```
com.miHoYo.Yuanshen 1 1
com.hypergryph.arknights 1 1
- -1 -1
* 1 0
```
