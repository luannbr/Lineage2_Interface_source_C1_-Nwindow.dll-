// URenderDevice: 46 slots from Engine.dll ??_7URenderDevice@@6BUObject@@@ (tools/gen_engine_decls.py, do not edit)
virtual INT Init() = 0;                                            //  25 +0x064
virtual INT SetRes(UViewport*, INT, INT, INT, INT, INT) = 0;       //  27 +0x06c
virtual INT SetRes(UViewport*, INT, INT, INT, INT, INT, INT) = 0;  //  26 +0x068
virtual INT GetRes(INT, INT*, INT*, INT*) = 0;                     //  28 +0x070
virtual INT GetRefreshRate(INT, INT, INT, INT, INT*) = 0;          //  29 +0x074
virtual void Exit(UViewport*) = 0;                                 //  30 +0x078
virtual void Flush(UViewport*) = 0;                                //  31 +0x07c
virtual void FlushResource(QWORD) = 0;                             //  32 +0x080
ENGINE_API virtual INT ResourceCached(QWORD);                      //  33 +0x084
virtual void UpdateGamma(UViewport*) = 0;                          //  34 +0x088
virtual void RestoreGamma() = 0;                                   //  35 +0x08c
virtual FRenderInterface* Lock(UViewport*, BYTE*, INT*) = 0;       //  36 +0x090
virtual void Unlock(FRenderInterface*) = 0;                        //  37 +0x094
virtual void Present(UViewport*) = 0;                              //  38 +0x098
virtual void ReadPixels(UViewport*, FColor*) = 0;                  //  39 +0x09c
virtual void HoldBackBuffer() = 0;                                 //  40 +0x0a0
virtual void DeleteAllTexture() = 0;                               //  41 +0x0a4
virtual INT IsUseTrilinear() = 0;                                  //  42 +0x0a8
virtual void SetUseTrilinear(INT) = 0;                             //  43 +0x0ac
virtual void SetEmulationMode(EHardwareEmulationMode) = 0;         //  44 +0x0b0
virtual FRenderCaps* GetRenderCaps() = 0;                          //  45 +0x0b4
