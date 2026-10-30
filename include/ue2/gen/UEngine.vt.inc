// UEngine: 48 slots from Engine.dll ??_7UEngine@@6BUObject@@@ (tools/gen_engine_decls.py, do not edit)
ENGINE_API virtual void Init(INT);                                              //  25 +0x064
ENGINE_API virtual void Exit();                                                 //  26 +0x068
ENGINE_API virtual void Flush(INT);                                             //  27 +0x06c
ENGINE_API virtual void UpdateGamma();                                          //  28 +0x070
ENGINE_API virtual void RestoreGamma();                                         //  29 +0x074
ENGINE_API virtual INT Key(UViewport*, EInputKey, _WORD);                       //  30 +0x078
ENGINE_API virtual INT InputEvent(UViewport*, EInputKey, EInputAction, FLOAT);  //  31 +0x07c
virtual void Draw(UViewport*, INT, BYTE*, INT*) = 0;                            //  32 +0x080
virtual void MouseDelta(UViewport*, DWORD, FLOAT, FLOAT) = 0;                   //  33 +0x084
virtual void MousePosition(UViewport*, DWORD, FLOAT, FLOAT) = 0;                //  34 +0x088
virtual void MouseWheel(UViewport*, DWORD, INT) = 0;                            //  35 +0x08c
virtual void Click(UViewport*, DWORD, FLOAT, FLOAT) = 0;                        //  36 +0x090
virtual void UnClick(UViewport*, DWORD, INT, INT) = 0;                          //  37 +0x094
virtual void SetClientTravel(UPlayer*, const TCHAR*, INT, ETravelType) = 0;     //  38 +0x098
ENGINE_API virtual INT ChallengeResponse(INT);                                  //  39 +0x09c
ENGINE_API virtual FLOAT GetMaxTickRate();                                      //  40 +0x0a0
ENGINE_API virtual void SetProgress(const TCHAR*, const TCHAR*, FLOAT);         //  41 +0x0a4
ENGINE_API virtual void edSetClickLocation(FVector&);                           //  42 +0x0a8
ENGINE_API virtual void edDrawAxisIndicator(FSceneNode*);                       //  43 +0x0ac
ENGINE_API virtual INT edcamMode(UViewport*);                                   //  44 +0x0b0
ENGINE_API virtual INT edcamTerrainBrush();                                     //  45 +0x0b4
ENGINE_API virtual INT edcamMouseControl(UViewport*);                           //  46 +0x0b8
ENGINE_API virtual void EdCallback(DWORD, INT, DWORD);                          //  47 +0x0bc
