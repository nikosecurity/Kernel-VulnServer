# Kernel-VulnServer
 Like VulnServer, but kernel-mode.
 Just like VulnServer, please do **not** use this software on your primary computer. Instead, opt for a virtual machine or a spare computer where you can play around with it. **This is an intentionally vulnerable server with no real usage!**

# Usage
 Create a Windows 7 x64 guest machine with your hypervisor of choice. Then, start the server in the virtual machine and load the driver (using OSRLoader or another utility such as System Informer/Process Hacker).
 By default, the server will use TCP port 4444, but this can be customized within the source code via the LISTENING_PORT macro.
 From here, you can attempt to exploit the kernel driver remotely!

# Implemented Bugs
 - Use-After-Free
 - Intentions on implementing more in the future, similarly to HEVD

# Building
 To build the program, you must have the [Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) installed in order to build the solution.
 Beyond that, the steps are provided below:
 1) Clone the repository (either download it manually or via CLI with git clone https://github.com/nikosecurity/Kernel-VulnServer.git)
 2) Open the solution and select either Release x64 or Debug x64 (32-bit is untested)
 3) Hover over "Build" and click Build Solution (or press F7)
 4) Done!

 On a side note, the project would use something like CMake and provide instructions for how to build with it, but there are two issues with this:
 1) This project is way too small for me to care enough to use CMake
 2) I literally don't know how to use CMake
 So, a simple Visual Studio solution will work for now.
