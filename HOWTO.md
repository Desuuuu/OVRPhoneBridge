# Table of Contents
* [Mobile application](#mobile-application)
  * [Installation](#installation)
  * [Configuration](#configuration)
* [PC application](#pc-application)
  * [Installation](#installation-1)
  * [Configuration](#configuration-1)
  * [Running](#running)

## Mobile application
### Installation
Transfer the [client's APK](https://github.com/Desuuuu/OVRPhoneBridge-Android/releases) to your phone.

Browse your files to where you put the APK, click it and follow the instructions to install it.

### Configuration
In the application settings, enter your computer's IP address and a password (used to encrypt communication).

*You can optionally edit the port which will be used, just make sure you use the same one on the PC application.*

Finally, enable whichever feature you want to use and grant permissions if prompted.

## PC application
### Installation
You can either use the installer or just extract the GZIP archive for the PC application.

When using the installer, the manifest will be automatically registered with OpenVR, meaning OVRPhoneBridge should appear in the applications tab of SteamVR and start automatically when launching SteamVR.

In the case you use the archive, you can install the manifest using the `install_manifest.bat` script in the application folder and achieve the same result.

*If you do not wish for OVRPhoneBridge to start automatically, do not install the manifest (or uninstall it using `uninstall_manifest.bat`). You will need to start `OVRPhoneBridge.exe` manually when you want to use the application.*

### Configuration
Start OpenVR.

*If you opted not to install the manifest, start `OVRPhoneBridge.exe` as well.*

Open the dashboard and click the `Phone Bridge` badge at the bottom.

In the `Settings` tab, enter the same password you used on the mobile application.

*If you edited the port on the mobile application, edit it here to the same value.*

You should now be able to start the server by clicking the `Start server` button.

At this point, you might have a firewall prompt on your desktop, make sure you accept it.

### Running
Start the service on your phone by pressing the toggle button on the application main page.

It will try to connect to your computer for around 5 minutes before giving up (unless you use the `Retry indefinitely` setting).

If you do not have SteamVR running, start it now.

The connection should now be established between your phone and your computer.
