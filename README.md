# 3D Game – DirectX 12

A custom 3D game project developed in **C++** using **DirectX 12**, focusing on low-level graphics programming and engine-style architecture.

This project demonstrates core real-time rendering concepts such as pipeline setup, PSO management, descriptor heaps, constant buffers, camera systems, instancing, and performance-aware rendering.

---

## 🚀 Features

- DirectX 12 graphics pipeline setup  
- Root Signature and Pipeline State Objects (PSO)  
- Descriptor Heaps and Constant Buffers  
- Camera system with keyboard and mouse input  
- Instancing for efficient rendering  
- Shader compilation and management (HLSL)  
- Modular, engine-oriented project structure  

---

## 🛠 Technologies

- **Language:** C++  
- **Graphics API:** DirectX 12  
- **Shaders:** HLSL  
- **IDE:** Visual Studio 2019 / 2022  
- **Platform:** Windows  

---

## 📁 Project Structure

3D-Game-Directx12/
├── assets/ # Models, textures, resources
├── shaders/ # HLSL shader files
├── source/ # Core engine and game source code
│ ├── engine/
│ ├── render/
│ ├── input/
│ ├── math/
│ └── main.cpp
├── README.md
└── 3DGame.sln


---

## 🧠 Overview

The project is built from scratch to explore DirectX 12 and engine-level systems:

- **Root Signatures** define how GPU resources are bound  
- **PSOs** describe fully configured pipeline states  
- **Descriptor Heaps** manage GPU-visible resources  
- **Constant Buffers** update transformation and camera data  
- **Instancing** allows efficient drawing of repeated objects  
- **Camera System** enables real-time navigation in 3D space  

---

## 🧪 Requirements

- Windows 10 or later  
- DirectX 12 compatible GPU  
- Visual Studio 2019 or 2022  
- Windows SDK (with DirectX 12 support)  

---

## ▶️ Build & Run

1. Clone the repository:
git clone https://github.com/ege-ozgur/3D-Game-Directx12.git

2. Open the solution file:
3DGame.sln


3. Select **Debug** or **Release** configuration  
4. Build the solution  
5. Run the application

---

## 🎮 Controls

- **W / A / S / D** – Move  
- **Mouse** – Look around  
- **ESC** – Exit  

---

## 📈 Future Improvements

- Advanced lighting (Phong / PBR)  
- Physics integration  
- Animation system  
- Material and texture system  
- Audio support  

---

## 📄 License

This project is released under the **MIT License**.

---

## 👤 Author

**Ege Özgür**  
📧 egeozgurbusiness@gmail.com  
🌐 https://ege-ozgur.github.io  

