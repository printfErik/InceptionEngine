# InceptionEngine

InceptionEngine is a toy 3D game engine. The active renderer bring-up path is D3D12.

## VS Code Build

You do not need the Visual Studio IDE. On Windows, D3D12 still needs a Windows SDK and an MSVC-compatible compiler/toolchain.

Install:

- VS Code
- CMake Tools extension
- CMake
- Ninja
- Windows SDK
- Either Visual Studio Build Tools or LLVM with `clang-cl`
- `dxc.exe` from Windows SDK, Vulkan SDK, or DirectX Shader Compiler

Open this folder in VS Code, then select:

- Configure preset: `windows-d3d12-debug`
- Build preset: `windows-d3d12-debug`
- Launch config: `InceptionEngine D3D12 Debug`

The executable path used by VS Code is:

```text
build/windows-d3d12-debug/src/InceptionEngine.exe
```

If CMake cannot find `dxc.exe`, set `DXC_PATH` to the directory that contains it, then reconfigure.
