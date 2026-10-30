// AActor: 145 slots from Engine.dll ??_7AActor@@6B@ (tools/gen_engine_decls.py, do not edit)
ENGINE_API virtual INT* GetOptimizedRepList(BYTE*, FPropertyRetirement*, INT*, UPackageMap*, UActorChannel*);  //  24 +0x060
ENGINE_API virtual APawn* GetPlayerPawn() const;                                                               //  25 +0x064
ENGINE_API virtual INT IsPlayer();                                                                             //  26 +0x068
ENGINE_API virtual FLOAT GetNetPriority(AActor*, FLOAT, FLOAT);                                                //  27 +0x06c
ENGINE_API virtual FLOAT WorldLightRadius() const;                                                             //  28 +0x070
ENGINE_API virtual INT Tick(FLOAT, ELevelTick);                                                                //  29 +0x074
ENGINE_API virtual void PostEditMove();                                                                        //  30 +0x078
ENGINE_API virtual void PostEditLoad();                                                                        //  31 +0x07c
ENGINE_API virtual void PreRaytrace();                                                                         //  32 +0x080
ENGINE_API virtual void PostRaytrace();                                                                        //  33 +0x084
ENGINE_API virtual void Spawned();                                                                             //  34 +0x088
ENGINE_API virtual void PreNetReceive();                                                                       //  35 +0x08c
ENGINE_API virtual void PostNetReceive();                                                                      //  36 +0x090
ENGINE_API virtual void PostNetReceiveLocation();                                                              //  37 +0x094
ENGINE_API virtual UMaterial* GetSkin(INT);                                                                    //  38 +0x098
ENGINE_API virtual INT ShouldTickInEntry();                                                                    //  39 +0x09c
ENGINE_API virtual void L2EventShow(INT);                                                                      //  40 +0x0a0
ENGINE_API virtual void L2EventPlay(INT);                                                                      //  41 +0x0a4
ENGINE_API virtual void LostChild(AActor*);                                                                    //  42 +0x0a8
ENGINE_API virtual void GainedChild(AActor*);                                                                  //  43 +0x0ac
ENGINE_API virtual INT CanBeAttacked();                                                                        //  44 +0x0b0
ENGINE_API virtual INT CanBeTold();                                                                            //  45 +0x0b4
ENGINE_API virtual void PostRender();                                                                          //  46 +0x0b8
ENGINE_API virtual void SetMeshes(INT, FName, INT);                                                            //  47 +0x0bc
ENGINE_API virtual void SetTexes(INT, FName, INT);                                                             //  48 +0x0c0
ENGINE_API virtual UMaterial* GetSubSkin(INT);                                                                 //  49 +0x0c4
ENGINE_API virtual UMaterial* GetRightHandSkin(INT);                                                           //  50 +0x0c8
ENGINE_API virtual UMaterial* GetLeftHandSkin(INT);                                                            //  51 +0x0cc
ENGINE_API virtual UMaterial* GetRightArmSkin(INT);                                                            //  52 +0x0d0
ENGINE_API virtual UMaterial* GetLeftArmSkin(INT);                                                             //  53 +0x0d4
ENGINE_API virtual UMaterial* GetCloakSkin(INT);                                                               //  54 +0x0d8
ENGINE_API virtual void CheckSameAnim();                                                                       //  55 +0x0dc
ENGINE_API virtual INT IsSameAnim();                                                                           //  56 +0x0e0
ENGINE_API virtual void ClearTexModifier();                                                                    //  57 +0x0e4
ENGINE_API virtual void SetAlphaTexModifier(BYTE);                                                             //  58 +0x0e8
ENGINE_API virtual void SetOverlayTexModifier(BYTE, BYTE, BYTE, BYTE);                                         //  59 +0x0ec
ENGINE_API virtual void SetOverlayDependAlphaTexModifier(BYTE, BYTE, BYTE);                                    //  60 +0x0f0
ENGINE_API virtual void SetSubtractTexModifier(BYTE, BYTE, BYTE);                                              //  61 +0x0f4
ENGINE_API virtual void SetAddTexModifier(BYTE, BYTE, BYTE);                                                   //  62 +0x0f8
ENGINE_API virtual void SetChangeColorTexModifier(BYTE, BYTE, BYTE);                                           //  63 +0x0fc
ENGINE_API virtual INT IsModifiedTexture();                                                                    //  64 +0x100
ENGINE_API virtual UModifier* GetModifiedTextureMaterial();                                                    //  65 +0x104
ENGINE_API virtual void SetActorViewType(EActorViewType, INT, BYTE);                                           //  66 +0x108
ENGINE_API virtual INT IsNeedTick();                                                                           //  67 +0x10c
ENGINE_API virtual void SetStaticMeshActorViewType(INT, INT);                                                  //  68 +0x110
ENGINE_API virtual FMatrix ConvertOrthMatrix(FMatrix, INT);                                                    //  69 +0x114
ENGINE_API virtual FMatrix CollisionToWorld();                                                                 //  70 +0x118
ENGINE_API virtual FCoords ToLocal() const;                                                                    //  71 +0x11c
ENGINE_API virtual FCoords ToWorld() const;                                                                    //  72 +0x120
ENGINE_API virtual FMatrix LocalToWorld() const;                                                               //  73 +0x124
ENGINE_API virtual FMatrix BillBoardLocalToWorld(FRotator) const;                                              //  74 +0x128
ENGINE_API virtual FMatrix WorldToLocal() const;                                                               //  75 +0x12c
ENGINE_API virtual void UpdateRenderData();                                                                    //  76 +0x130
ENGINE_API virtual void PostScriptDestroyed();                                                                 //  77 +0x134
ENGINE_API virtual INT ShouldTrace(AActor*, DWORD);                                                            //  78 +0x138
ENGINE_API virtual UPrimitive* GetPrimitive();                                                                 //  79 +0x13c
ENGINE_API virtual void NotifyBump(AActor*);                                                                   //  80 +0x140
ENGINE_API virtual void SetBase(AActor*, FVector, INT);                                                        //  81 +0x144
ENGINE_API virtual void NotifyAnimEnd(INT);                                                                    //  82 +0x148
ENGINE_API virtual void UpdateAnimation(FLOAT);                                                                //  83 +0x14c
ENGINE_API virtual void StartAnimPoll();                                                                       //  84 +0x150
ENGINE_API virtual INT CheckAnimFinished(INT);                                                                 //  85 +0x154
ENGINE_API virtual INT CheckOwnerUpdated();                                                                    //  86 +0x158
ENGINE_API virtual void TickAuthoritative(FLOAT);                                                              //  87 +0x15c
ENGINE_API virtual void TickSimulated(FLOAT);                                                                  //  88 +0x160
ENGINE_API virtual void TickSpecial(FLOAT);                                                                    //  89 +0x164
ENGINE_API virtual INT PlayerControlled();                                                                     //  90 +0x168
ENGINE_API virtual INT IsNetRelevantFor(APlayerController*, AActor*, FVector);                                 //  91 +0x16c
ENGINE_API virtual void RenderEditorInfo(FLevelSceneNode*, FRenderInterface*, FDynamicActor*);                 //  92 +0x170
ENGINE_API virtual void RenderEditorSelected(FLevelSceneNode*, FRenderInterface*, FDynamicActor*);             //  93 +0x174
ENGINE_API virtual FLOAT GetAmbientVolume(FLOAT);                                                              //  94 +0x178
ENGINE_API virtual void SetZone(INT, INT);                                                                     //  95 +0x17c
ENGINE_API virtual void SetVolumes();                                                                          //  96 +0x180
ENGINE_API virtual void PostBeginPlay();                                                                       //  97 +0x184
ENGINE_API virtual void setPhysics(BYTE, AActor*, FVector);                                                    //  98 +0x188
ENGINE_API virtual void performPhysics(FLOAT);                                                                 //  99 +0x18c
ENGINE_API virtual void BoundProjectileVelocity();                                                             // 100 +0x190
ENGINE_API virtual void processHitWall(FVector, AActor*);                                                      // 101 +0x194
ENGINE_API virtual void processLanded(FVector, AActor*, FLOAT, INT);                                           // 102 +0x198
ENGINE_API virtual void physFalling(FLOAT, INT);                                                               // 103 +0x19c
ENGINE_API virtual FRotator FindSlopeRotation(FVector, FRotator);                                              // 104 +0x1a0
ENGINE_API virtual void SmoothHitWall(FVector, AActor*);                                                       // 105 +0x1a4
ENGINE_API virtual void stepUp(FVector, FVector, FVector, FCheckResult&);                                      // 106 +0x1a8
ENGINE_API virtual INT ShrinkCollision(AActor*);                                                               // 107 +0x1ac
ENGINE_API virtual _McdModel* getKModel() const;                                                               // 108 +0x1b0
ENGINE_API virtual void physKarma(FLOAT);                                                                      // 109 +0x1b4
ENGINE_API virtual void preKarmaStep(FLOAT);                                                                   // 110 +0x1b8
ENGINE_API virtual void postKarmaStep();                                                                       // 111 +0x1bc
ENGINE_API virtual void L2HitWall(FVector, AActor*);                                                           // 112 +0x1c0
ENGINE_API virtual void L2Touch(AActor*);                                                                      // 113 +0x1c4
ENGINE_API virtual void L2Explode(FVector, AActor*);                                                           // 114 +0x1c8
ENGINE_API virtual USound* GetAttackItemSound(FLOAT&, FLOAT&);                                                 // 115 +0x1cc
ENGINE_API virtual USound* GetDefenseItemSound(FLOAT&, FLOAT&);                                                // 116 +0x1d0
ENGINE_API virtual USound* GetShieldItemSound(FLOAT&, FLOAT&);                                                 // 117 +0x1d4
ENGINE_API virtual USound* GetDamageSound(FLOAT&, FLOAT&);                                                     // 118 +0x1d8
ENGINE_API virtual USound* GetAttackVoiceSound(FLOAT);                                                         // 119 +0x1dc
ENGINE_API virtual void AttackedNotify(APawn*, AActor*, INT, INT, INT);                                        // 120 +0x1e0
ENGINE_API virtual void AssociateAttackedNotify(APawn*, AActor*, INT, INT, INT, INT, INT, INT, INT, INT);      // 121 +0x1e4
ENGINE_API virtual INT IsRendered();                                                                           // 122 +0x1e8
ENGINE_API virtual FName GetRHandBoneName();                                                                   // 123 +0x1ec
ENGINE_API virtual FName GetLHandBoneName();                                                                   // 124 +0x1f0
ENGINE_API virtual FName GetRArmBoneName();                                                                    // 125 +0x1f4
ENGINE_API virtual FName GetLArmBoneName();                                                                    // 126 +0x1f8
ENGINE_API virtual FName GetCapeBoneName();                                                                    // 127 +0x1fc
ENGINE_API virtual FName GetSpineBoneName();                                                                   // 128 +0x200
ENGINE_API virtual INT AddMyMarker(AActor*);                                                                   // 129 +0x204
ENGINE_API virtual void ClearMarker();                                                                         // 130 +0x208
ENGINE_API virtual AActor* AssociatedLevelGeometry();                                                          // 131 +0x20c
ENGINE_API virtual INT HasAssociatedLevelGeometry(AActor*);                                                    // 132 +0x210
ENGINE_API virtual void PlayAnim(INT, FName, FLOAT, FLOAT, INT);                                               // 133 +0x214
ENGINE_API virtual void CheckForErrors();                                                                      // 134 +0x218
ENGINE_API virtual void PrePath();                                                                             // 135 +0x21c
ENGINE_API virtual void PostPath();                                                                            // 136 +0x220
ENGINE_API virtual AActor* GetProjectorBase();                                                                 // 137 +0x224
ENGINE_API virtual INT IsABrush();                                                                             // 138 +0x228
ENGINE_API virtual INT IsAMover();                                                                             // 139 +0x22c
ENGINE_API virtual INT IsAVolume();                                                                            // 140 +0x230
ENGINE_API virtual INT IsAPlayerController();                                                                  // 141 +0x234
ENGINE_API virtual INT IsAPawn();                                                                              // 142 +0x238
ENGINE_API virtual INT IsAProjectile();                                                                        // 143 +0x23c
ENGINE_API virtual APlayerController* GetTopPlayerController();                                                // 144 +0x240
