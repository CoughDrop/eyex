## EyeX for Node

This is a node (Windows-only) node module that can listen in for eye tracking events
generated by the EyeX libraries installed on a Windows device.

### Technical Notes
We use this library as part of our Electron app for CoughDrop. In order for it to work,
we place the correct (x86 or x64) `Tobii.EyeX.Client.dll` file in the root folder of our
app. For compiling, you can add the necessary resources from the `lib` and `include` folders
in the C++ SDK as well. You should find the EyeX SDKs at [http://developer.tobii.com/].

Also, I was having serious trouble getting the eyex.node file in the build/Release folder to work
correctly with Electron, so it's currently ignoring that file and instead using the one in the root
folder. I hope to fix this and stop embarrassing myself someday, but boy am I not
a low-lever programmer.

### License

MIT
