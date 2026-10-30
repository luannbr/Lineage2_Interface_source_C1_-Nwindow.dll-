// UAudioSubsystem: 49 slots from Engine.dll ??_7UAudioSubsystem@@6BUObject@@@ (tools/gen_engine_decls.py, do not edit)
virtual INT Init() = 0;                                                                              //  25 +0x064
virtual void SetViewport(UViewport*) = 0;                                                            //  26 +0x068
virtual void Update(FSceneNode*) = 0;                                                                //  27 +0x06c
virtual void RegisterSound(USound*) = 0;                                                             //  28 +0x070
virtual void UnregisterSound(USound*) = 0;                                                           //  29 +0x074
virtual INT PlaySoundW(AActor*, INT, USound*, FVector, FLOAT, FLOAT, FLOAT, INT, FLOAT, FLOAT) = 0;  //  30 +0x078
virtual INT StopSound(AActor*, USound*) = 0;                                                         //  31 +0x07c
virtual INT IsPlaying(AActor*, USound*) = 0;                                                         //  32 +0x080
virtual void SetSoundVolume(FLOAT) = 0;                                                              //  33 +0x084
virtual void SetMusicVolume(FLOAT) = 0;                                                              //  34 +0x088
virtual FLOAT GetSoundVolume() = 0;                                                                  //  35 +0x08c
virtual FLOAT GetMusicVolume() = 0;                                                                  //  36 +0x090
virtual INT IsMusicPlaying() = 0;                                                                    //  37 +0x094
virtual INT CheckSoundResource(USound*) = 0;                                                         //  38 +0x098
virtual INT PlayMusic(FString, FLOAT, INT) = 0;                                                      //  39 +0x09c
virtual INT StopMusic(INT, FLOAT) = 0;                                                               //  40 +0x0a0
virtual INT StopAllMusic(FLOAT) = 0;                                                                 //  41 +0x0a4
virtual void ChangeVoiceChatter(DWORD, DWORD, INT) = 0;                                              //  42 +0x0a8
virtual void EnterVoiceChat() = 0;                                                                   //  43 +0x0ac
virtual void LeaveVoiceChat() = 0;                                                                   //  44 +0x0b0
virtual INT LowQualitySound() = 0;                                                                   //  45 +0x0b4
virtual void NoteDestroy(AActor*) = 0;                                                               //  46 +0x0b8
virtual UViewport* GetViewport() = 0;                                                                //  47 +0x0bc
ENGINE_API virtual void CleanUp();                                                                   //  48 +0x0c0
