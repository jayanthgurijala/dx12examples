# DX12 Samples

A collection of DirectX 12 sample projects demonstrating different graphics pipelines and modern GPU features.

---

## ✨ Features

- Simple VS–PS pipeline
- Tessellation
- Mesh Shading
- Ray Tracing

---

## 🎯 Goals

- Learn DirectX 12 with minimal abstraction (no engine layer)
- Provide high-quality test content for:
  - Driver developers
  - Tool developers
- Enable automation (planned):
  - Deterministic frame output
  - Image dumping
  - Fixed frame rate execution

---

### 📺 Videos
[Introduction](https://www.youtube.com/watch?v=xazLPw_QeAg)




[RayTracing Deer Texturing Debugging](https://www.youtube.com/watch?v=E4kxmSjIjUs)

## 🛠 Requirements

- Windows 10
- Visual Studio 2026
- DirectX 12 compatible GPU
- Latest Graphics Drivers

---

## 🚀 Getting Started

### 1️⃣ Clone the repository

```bash
git clone https://github.com/jayanthgurijala/dx12examples.git
cd dx12examples
git submodule init
git submodule update
```

### 2️⃣ Open the solution
### 3️⃣ Build and Run

- **Dx12SampleBase** is a shared library and does not run on its own.  
- To execute a sample, select **any of the other projects** as the **Startup Project** in Visual Studio.  
- Choose your build configuration (`Debug` or `Release`) and press **F5** to run.


### Third-Party Libraries

This project uses the following third-party libraries:

- **Dear ImGui** – Immediate Mode GUI library  
  https://github.com/ocornut/imgui  
  License: MIT

- **tinygltf** – glTF loader and parser  
  https://github.com/syoyo/tinygltf  
  License: MIT









