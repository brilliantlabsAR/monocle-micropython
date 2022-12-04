Get Started with Monocle Mobile App
===================================

Configuration
-------------

You should place a configuration file at the project root: ``<project_root>/.env``. There is an example ``.env.example`` file to reference for the expected format.
The .env file has the following params:

1. LOG_LEVEL=DEBUG|INFO|WARN|ERROR (optional, defaults to INFO)
2. ENVIRONMENT_TYPE=DEBUG|STAGE|PRODUCTION (optional, defaults to PRODUCTION)

Set Up
------

1. Make sure you have a recent version of ``npm`` installed.
2. Make sure ``yarn`` is installed, e.g. ``npm i -g yarn``.
3. In the project root, install all the npm packages: ``yarn install``.
4. For iOS, make sure you have Cocoapods installed: https://guides.cocoapods.org/using/getting-started.html
5. Install the pods: ``cd ios``, then ``pod install``.
6. You can now open the ``xcworkspace`` file and then run the app using Xcode.
7. For Android, you should generate your own keystore, e.g.: ``keytool -genkey -v -keystore ./android/app/debug.keystore -storepass android -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000``
8. You may need to run ``yarn start`` in a separate terminal window to manually start Metro before running Android in debug mode.
9. Run ``yarn android`` to start the app on Android device/emulator.

iOS Distribution Builds
-----------------------

You can use the standard approach in Xcode to make builds--nothing special is required.

Android Distribution Builds
---------------------------

Put a file ``android/app/gradle.properties`` with the following contents: 

.. code::

    release_keystore=your_key_name.keystore
    release_keystore_password=your_key_store_password
    release_key_alias=your_key_alias
    release_key_password=your_key_password

Optionally, you could pass the parameters from command-line with the ``-P`` option. For example, ``./gradlew assembleRelease -Prelease_key_password=keypass``.

There is a bash script that's provided, ``build_android.sh``, which will automate making builds (there are a few extra steps that are necessary to make the build work, which this script takes care of).
