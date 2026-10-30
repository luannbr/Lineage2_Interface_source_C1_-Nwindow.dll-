// UGameEngine: 57 slots from Engine.dll ??_7UGameEngine@@6BUObject@@@ (tools/gen_engine_decls.py, do not edit)
ENGINE_API virtual INT Browse(FURL, const TMap<FString, FString>*, FString&);                              //  48 +0x0c0
ENGINE_API virtual ULevel* LoadMap(const FURL&, UPendingLevel*, const TMap<FString, FString>*, FString&);  //  49 +0x0c4
ENGINE_API virtual void SaveGame(INT);                                                                     //  50 +0x0c8
ENGINE_API virtual void CancelPending();                                                                   //  51 +0x0cc
ENGINE_API virtual void PaintProgress();                                                                   //  52 +0x0d0
ENGINE_API virtual void UpdateConnectingMessage();                                                         //  53 +0x0d4
ENGINE_API virtual void BuildServerMasterMap(UNetDriver*, ULevel*);                                        //  54 +0x0d8
ENGINE_API virtual void NotifyLevelChange();                                                               //  55 +0x0dc
ENGINE_API virtual void ContinueServerClose();                                                             //  56 +0x0e0
