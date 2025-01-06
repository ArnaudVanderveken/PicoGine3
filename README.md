# PicoGine3
 
This new project is the continuation of my previous work on PicoGine(https://github.com/ArnaudVanderveken/PicoGine). In this new version, I will focus more on graphics and rendering, rather than a broader engine, since this is the domain I want to specialize into. Therefore, I will rely on more libraries to build the core of the engine, such as a entity-component system like Flecs for example. 
The main goal is to incrementally add new rendering features using both Vulkan and DirectX 12 under a common abstraction.
To make the shader compilation easier, I made another project called PicoGineSC. More info on its project page (https://github.com/ArnaudVanderveken/PicoGineSC).

## Win32, window, and inputs
Initially, I was planning to use the SDL library to create the window as well as handle inputs. Because I will not use other features from SDL, I decided to still implement it myself using Win32 since I've done it multiple times already and might be even faster than implementing SDL.
The window supports resizing, minimizing, maximizing, and toggling full screen borderless mode. It also supports multiple monitors, and can move the window across screens with the Windows shortcuts (Win + Shift + Arrows) even in full screen borderless mode.
For the inputs, I used raw input devices for the mouse and keyboard to try something new. I also added controller support using the Xinput library. 

## Vulkan and DirectX 12
The project will support both Vulkan and DirectX 12. I made separate build targets which both have a preprocessor define for their specific API. This allows me to batch build all targets to ensure the changes made with one API don't beak the other.
At the moment, only a basic Vulkan implementation is available, which was made following the vulkan-tutorial.com (https://vulkan-tutorial.com), similarly to my previous Vulkan Renderer project (https://github.com/ArnaudVanderveken/VulkanTest) except this time I used HLSL for the shaders instead of GLSL. The next step is to cleanup and restructure the code properly, then start working on abstracting all the key elements. Then, I'll make a similar DirectX 12 implementation following this 3dgep: Learning DirectX 12 (https://www.3dgep.com/learning-directx-12-1/) guide.
Once the barebone core of both API works under the same abstraction, I will start implementing more advanced topics, most likely in Vulkan first because I have more reference material for that API.
