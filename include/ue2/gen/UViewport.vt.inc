// UViewport: 42 slots from Engine.dll ??_7UViewport@@6BUObject@@@ (tools/gen_engine_decls.py, do not edit)
ENGINE_API virtual INT Lock(BYTE*, INT*);                              //  25 +0x064
ENGINE_API virtual void Unlock();                                      //  26 +0x068
ENGINE_API virtual void Present();                                     //  27 +0x06c
ENGINE_API virtual INT SetDrag(INT);                                   //  28 +0x070
virtual INT IsFullscreen() = 0;                                        //  29 +0x074
virtual INT ResizeViewport(DWORD, INT, INT, INT) = 0;                  //  30 +0x078
virtual void SetModeCursor() = 0;                                      //  31 +0x07c
virtual void UpdateWindowFrame() = 0;                                  //  32 +0x080
virtual void OpenWindow(DWORD, INT, INT, INT, INT, INT) = 0;           //  33 +0x084
virtual void CloseWindow() = 0;                                        //  34 +0x088
virtual void UpdateInput(INT, FLOAT) = 0;                              //  35 +0x08c
virtual void* GetWindow() = 0;                                         //  36 +0x090
ENGINE_API virtual void* GetServer();                                  //  37 +0x094
virtual void SetMouseCapture(INT, INT, INT) = 0;                       //  38 +0x098
virtual void Repaint(INT) = 0;                                         //  39 +0x09c
ENGINE_API virtual void TryRenderDevice(const TCHAR*, INT, INT, INT);  //  40 +0x0a0
virtual TCHAR* GetLocalizedKeyName(EInputKey) = 0;                     //  41 +0x0a4
