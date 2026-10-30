# Progresso da decompilação

Gerado por `tools/build_db.py` — não editar à mão.

- Funções a decompilar (game + init): **3653** (756,335 bytes)
- Reimplementadas (hook ativo): **2744** (721,710 bytes, 95.42% do código)
- Verificadas no jogo: **1360**
- Sem gancho, com motivo na coluna notes do banco (docs/notas/Restante.md):
  - convenção (deleting destructors, InternalConstructor): **359** (9,115 bytes)
  - só na fase 5 (inicialização estática, DllMain): **543** (25,247 bytes)
  - mortas no modo gancho (db/morto.csv): **7** (263 bytes)
- Sem gancho e sem motivo (todo): **0** (0 bytes)

| kind | funções |
|---|---:|
| crt | 481 |
| eh | 1265 |
| fragment | 7 |
| game | 3111 |
| init | 542 |
| thunk | 62 |

## Por classe

| classe | funções | feitas | sem gancho | todo | bytes | % bytes |
|---|---:|---:|---:|---:|---:|---:|
| FL2FontPage | 5 | 5 | 0 | 0 | 216 | 100.0% |
| FL2SystemMsgData | 1 | 1 | 0 | 0 | 9 | 100.0% |
| FParser | 2 | 2 | 0 | 0 | 12 | 100.0% |
| FString | 1 | 1 | 0 | 0 | 12 | 100.0% |
| NActionItem | 9 | 9 | 0 | 0 | 248 | 100.0% |
| NCAbnormalStatusWnd | 19 | 14 | 5 | 0 | 3,305 | 94.2% |
| NCAbstractMain | 5 | 1 | 4 | 0 | 252 | 31.7% |
| NCActionItemListWnd | 11 | 6 | 5 | 0 | 1,524 | 87.5% |
| NCActionItemWnd | 21 | 16 | 5 | 0 | 4,319 | 95.4% |
| NCActionLinker | 10 | 5 | 5 | 0 | 609 | 68.3% |
| NCActionWnd | 13 | 8 | 5 | 0 | 1,256 | 84.8% |
| NCAudioOptionWnd | 13 | 8 | 5 | 0 | 2,773 | 93.1% |
| NCAuthWnd | 17 | 12 | 5 | 0 | 2,377 | 92.0% |
| NCAutoLogWnd | 9 | 4 | 5 | 0 | 835 | 77.1% |
| NCBBSHtmlViewer | 20 | 15 | 5 | 0 | 3,343 | 94.3% |
| NCBoardWnd | 19 | 14 | 5 | 0 | 3,426 | 94.4% |
| NCButton | 21 | 16 | 5 | 0 | 2,881 | 93.4% |
| NCCandidateBox | 25 | 20 | 5 | 0 | 3,832 | 94.9% |
| NCChatFilterWnd | 17 | 12 | 5 | 0 | 6,809 | 97.2% |
| NCChatListBox | 12 | 7 | 5 | 0 | 1,632 | 88.3% |
| NCChatWnd | 33 | 28 | 5 | 0 | 15,762 | 98.8% |
| NCCheckBox | 11 | 6 | 5 | 0 | 1,316 | 84.8% |
| NCChineseIME | 22 | 17 | 5 | 0 | 8,649 | 97.8% |
| NCCmdTarget | 5 | 1 | 4 | 0 | 252 | 31.7% |
| NCComboBox | 21 | 16 | 5 | 0 | 2,792 | 93.2% |
| NCCompositeBox | 16 | 11 | 5 | 0 | 1,831 | 89.6% |
| NCConsole | 39 | 34 | 5 | 0 | 17,360 | 98.9% |
| NCCoolTimeIcon | 10 | 10 | 0 | 0 | 1,547 | 100.0% |
| NCCountSpin | 13 | 8 | 5 | 0 | 1,551 | 87.3% |
| NCDebugWnd | 15 | 10 | 5 | 0 | 1,159 | 83.5% |
| NCDeliverItemWnd | 16 | 11 | 5 | 0 | 9,685 | 98.0% |
| NCDeliverWnd | 21 | 16 | 5 | 0 | 3,945 | 95.2% |
| NCDetailStatusWnd | 9 | 4 | 5 | 0 | 6,395 | 97.0% |
| NCDialogBox | 25 | 20 | 5 | 0 | 8,589 | 97.8% |
| NCEditBox | 39 | 34 | 5 | 0 | 14,492 | 98.6% |
| NCEquipItemWnd | 29 | 24 | 5 | 0 | 16,031 | 98.8% |
| NCExButton | 11 | 6 | 5 | 0 | 463 | 58.7% |
| NCFileLinker | 7 | 2 | 5 | 0 | 334 | 42.2% |
| NCFrameCtrl | 19 | 14 | 5 | 0 | 2,279 | 91.6% |
| NCGameOptionWnd | 17 | 12 | 5 | 0 | 6,553 | 97.1% |
| NCGDevice | 34 | 29 | 5 | 0 | 3,212 | 94.2% |
| NCGMActionWnd | 9 | 4 | 5 | 0 | 1,049 | 81.8% |
| NCGMDetailStatusWnd | 10 | 5 | 5 | 0 | 5,622 | 96.6% |
| NCGMEquipItemWnd | 9 | 4 | 5 | 0 | 372 | 48.1% |
| NCGMInvenItemWnd | 9 | 4 | 5 | 0 | 387 | 50.6% |
| NCGMInventoryWnd | 13 | 8 | 5 | 0 | 1,893 | 89.9% |
| NCGMMagicSkillWnd | 12 | 7 | 5 | 0 | 862 | 77.8% |
| NCGMPetitionWnd | 13 | 8 | 5 | 0 | 2,104 | 90.9% |
| NCGMPledgeWnd | 10 | 5 | 5 | 0 | 1,338 | 85.7% |
| NCGMQuestWnd | 11 | 6 | 5 | 0 | 1,996 | 90.4% |
| NCGMSkillItemWnd | 10 | 5 | 5 | 0 | 405 | 52.8% |
| NCGMSkillWnd | 8 | 3 | 5 | 0 | 879 | 78.3% |
| NCGMStoreWnd | 10 | 5 | 5 | 0 | 522 | 63.4% |
| NCGMWnd | 18 | 13 | 5 | 0 | 3,814 | 95.0% |
| NChatBalloon | 1 | 1 | 0 | 0 | 8 | 100.0% |
| NCHeaderCtrl | 11 | 6 | 5 | 0 | 1,015 | 81.2% |
| NCHtmlButton | 11 | 6 | 5 | 0 | 1,099 | 82.6% |
| NCHtmlComboBox | 13 | 8 | 5 | 0 | 2,045 | 90.7% |
| NCHtmlEdit | 12 | 7 | 5 | 0 | 2,366 | 91.8% |
| NCHtmlMultiEdit | 16 | 11 | 5 | 0 | 7,354 | 97.4% |
| NCHtmlPushButton | 9 | 4 | 5 | 0 | 1,046 | 81.7% |
| NCHtmlSliderBar | 9 | 4 | 5 | 0 | 732 | 73.9% |
| NCHtmlViewer | 37 | 32 | 5 | 0 | 5,564 | 96.6% |
| NCIcon | 15 | 10 | 5 | 0 | 1,716 | 88.9% |
| NCIME | 21 | 16 | 5 | 0 | 1,206 | 84.0% |
| NCIMEManager | 23 | 18 | 5 | 0 | 2,100 | 90.8% |
| NCInvenItemWnd | 23 | 18 | 5 | 0 | 13,230 | 98.6% |
| NCInventoryWnd | 19 | 14 | 5 | 0 | 3,861 | 95.1% |
| NCItemWnd | 28 | 23 | 5 | 0 | 3,341 | 94.3% |
| NCJapaneseIME | 20 | 15 | 5 | 0 | 5,501 | 96.5% |
| NCJHtmlEdit | 7 | 2 | 5 | 0 | 315 | 37.5% |
| NCKoreanIME | 13 | 8 | 5 | 0 | 4,141 | 95.3% |
| NCListBox | 24 | 19 | 5 | 0 | 3,338 | 94.3% |
| NCListCtrl | 35 | 30 | 5 | 0 | 5,411 | 96.5% |
| NCLoadingWnd | 9 | 4 | 5 | 0 | 691 | 72.4% |
| NCLobbyWnd | 19 | 14 | 5 | 0 | 3,022 | 93.7% |
| NCLoginWnd | 13 | 8 | 5 | 0 | 2,687 | 92.9% |
| NCMagicSkillWnd | 13 | 8 | 5 | 0 | 1,235 | 84.5% |
| NCMainWnd | 15 | 10 | 5 | 0 | 2,701 | 92.9% |
| NCMatchListCtrl | 16 | 11 | 5 | 0 | 2,914 | 93.4% |
| NCMatchOptionWnd | 14 | 9 | 5 | 0 | 2,804 | 93.2% |
| NCMatchWnd | 18 | 13 | 5 | 0 | 4,633 | 95.9% |
| NCMenuWnd | 15 | 10 | 5 | 0 | 1,817 | 89.5% |
| NCMessageWnd | 11 | 6 | 5 | 0 | 662 | 71.1% |
| NCMinimapWnd | 22 | 17 | 5 | 0 | 4,456 | 95.7% |
| NCMSView_CharacterSelectWnd | 12 | 7 | 5 | 0 | 2,669 | 92.8% |
| NCMSView_InvenItemWnd | 10 | 5 | 5 | 0 | 802 | 76.2% |
| NCMSViewrWnd | 18 | 13 | 5 | 0 | 8,038 | 97.6% |
| NCMultiEdit | 44 | 39 | 5 | 0 | 24,766 | 99.2% |
| NCNoticeInfoWnd | 12 | 7 | 5 | 0 | 624 | 69.4% |
| NCNoticeWnd | 15 | 10 | 5 | 0 | 1,405 | 86.4% |
| NCNPCHtmlViewer | 12 | 7 | 5 | 0 | 972 | 80.3% |
| NCNpcView_CharacterSelect_List | 19 | 14 | 5 | 0 | 1,908 | 90.0% |
| NCNpcView_CharacterSelectWnd | 17 | 12 | 5 | 0 | 5,998 | 96.8% |
| NCNpcView_InvenItemWnd | 11 | 6 | 5 | 0 | 969 | 80.3% |
| NCNpcViewrWnd | 18 | 13 | 5 | 0 | 4,080 | 95.3% |
| NCObject | 21 | 16 | 5 | 0 | 850 | 77.4% |
| NCommandParser | 2 | 2 | 0 | 0 | 135 | 100.0% |
| NConsoleWnd | 431 | 422 | 9 | 0 | 76,975 | 99.7% |
| NControl | 17 | 12 | 5 | 0 | 737 | 74.1% |
| NCOptionWnd | 15 | 10 | 5 | 0 | 2,623 | 92.7% |
| NCPartyStatusWnd | 12 | 7 | 5 | 0 | 1,249 | 84.5% |
| NCPartyWnd | 18 | 13 | 5 | 0 | 1,987 | 90.4% |
| NCPawnCreateWnd | 21 | 16 | 5 | 0 | 3,647 | 94.8% |
| NCPawnInfoWnd | 10 | 5 | 5 | 0 | 599 | 68.1% |
| NCPawnSelectWnd | 18 | 13 | 5 | 0 | 4,047 | 95.3% |
| NCPawnSetupWnd | 26 | 21 | 5 | 0 | 9,497 | 98.0% |
| NCPawnView_CharacterSelect_List | 12 | 7 | 5 | 0 | 796 | 76.0% |
| NCPawnView_CharacterSelectWnd | 13 | 8 | 5 | 0 | 3,128 | 93.9% |
| NCPawnView_InvenItemWnd | 10 | 5 | 5 | 0 | 821 | 76.7% |
| NCPawnView_InvenTextWnd | 10 | 5 | 5 | 0 | 962 | 80.1% |
| NCPawnViewr_HairChangeWnd | 16 | 11 | 5 | 0 | 7,530 | 97.5% |
| NCPawnViewr_MixAnimWnd | 15 | 10 | 5 | 0 | 2,825 | 93.2% |
| NCPawnViewr_Simulator | 15 | 10 | 5 | 0 | 4,794 | 96.0% |
| NCPawnViewrWnd | 24 | 19 | 5 | 0 | 9,130 | 97.9% |
| NCPetActionWnd | 11 | 6 | 5 | 0 | 964 | 80.2% |
| NCPetDetailStatusWnd | 9 | 4 | 5 | 0 | 1,952 | 90.2% |
| NCPetInvenItemWnd | 24 | 19 | 5 | 0 | 11,437 | 98.3% |
| NCPetInventoryWnd | 11 | 6 | 5 | 0 | 675 | 71.7% |
| NCPetitionListBox | 11 | 6 | 5 | 0 | 1,335 | 85.7% |
| NCPetitionWnd | 15 | 10 | 5 | 0 | 1,652 | 88.4% |
| NCPetStatusWnd | 12 | 7 | 5 | 0 | 1,476 | 86.9% |
| NCPetWnd | 17 | 12 | 5 | 0 | 3,981 | 95.2% |
| NCPlayerStatusWnd | 14 | 9 | 5 | 0 | 2,961 | 93.5% |
| NCPledgeWnd | 30 | 25 | 5 | 0 | 6,869 | 97.2% |
| NCPrivateBuyItemWnd | 21 | 16 | 5 | 0 | 15,791 | 98.8% |
| NCPrivateBuyWnd | 24 | 19 | 5 | 0 | 6,623 | 97.1% |
| NCPrivateShopItemWnd | 19 | 14 | 5 | 0 | 14,509 | 98.7% |
| NCPrivateShopWnd | 23 | 18 | 5 | 0 | 6,018 | 96.8% |
| NCProgressCtrl | 15 | 10 | 5 | 0 | 1,055 | 81.9% |
| NCPushButton | 13 | 8 | 5 | 0 | 743 | 74.3% |
| NCQuestInfoWnd | 18 | 13 | 5 | 0 | 2,032 | 90.6% |
| NCQuestItemWnd | 16 | 11 | 5 | 0 | 2,904 | 93.4% |
| NCQuestWnd | 20 | 15 | 5 | 0 | 2,743 | 93.0% |
| NCRadarWnd | 11 | 6 | 5 | 0 | 551 | 65.3% |
| NCRadioButton | 13 | 8 | 5 | 0 | 1,052 | 81.8% |
| NCResizeFrame | 12 | 7 | 5 | 0 | 830 | 76.7% |
| NCRestartMenuWnd | 19 | 14 | 5 | 0 | 2,698 | 92.9% |
| NCScrollWnd | 19 | 14 | 5 | 0 | 2,392 | 92.0% |
| NCSelectCtrl | 15 | 10 | 5 | 0 | 1,606 | 88.1% |
| NCSelectDeliverWnd | 17 | 12 | 5 | 0 | 1,769 | 89.2% |
| NCServerSelectListCtrl | 10 | 5 | 5 | 0 | 1,034 | 81.5% |
| NCServerSelectWnd | 30 | 25 | 5 | 0 | 6,046 | 96.8% |
| NCShopItemWnd | 17 | 12 | 5 | 0 | 11,918 | 98.4% |
| NCShopWnd | 23 | 18 | 5 | 0 | 5,989 | 96.8% |
| NCShortCutWnd | 33 | 28 | 5 | 0 | 12,144 | 98.4% |
| NCSiegeDateInfoWnd | 16 | 11 | 5 | 0 | 1,793 | 89.3% |
| NCSiegeInfoListCtrl | 14 | 9 | 5 | 0 | 1,740 | 89.0% |
| NCSiegeInfoMainWnd | 18 | 13 | 5 | 0 | 3,064 | 93.8% |
| NCSiegePartInfoWnd | 20 | 15 | 5 | 0 | 4,274 | 95.5% |
| NCSkillItemWnd | 18 | 13 | 5 | 0 | 5,508 | 96.5% |
| NCSkillWnd | 12 | 7 | 5 | 0 | 1,152 | 83.4% |
| NCSliderBar | 17 | 12 | 5 | 0 | 1,340 | 85.7% |
| NCStatusWnd | 12 | 7 | 5 | 0 | 590 | 67.3% |
| NCStoreItemWnd | 18 | 13 | 5 | 0 | 9,671 | 98.0% |
| NCStoreWnd | 25 | 20 | 5 | 0 | 5,438 | 96.5% |
| NCSummonedActionWnd | 9 | 4 | 5 | 0 | 775 | 75.4% |
| NCSummonedDetailStatusWnd | 8 | 3 | 5 | 0 | 343 | 44.3% |
| NCSummonedStatusWnd | 14 | 9 | 5 | 0 | 1,536 | 87.4% |
| NCSummonedWnd | 11 | 6 | 5 | 0 | 2,244 | 91.5% |
| NCSystemMenuWnd | 16 | 11 | 5 | 0 | 2,026 | 90.6% |
| NCTabControl | 16 | 11 | 5 | 0 | 2,347 | 91.9% |
| NCTaiwanIME | 19 | 14 | 5 | 0 | 6,539 | 97.0% |
| NCTargetStatusWnd | 15 | 10 | 5 | 0 | 3,629 | 94.7% |
| NCTestHtmlViewer | 10 | 5 | 5 | 0 | 846 | 77.4% |
| NCTestUIWnd | 9 | 4 | 5 | 0 | 602 | 68.3% |
| NCTradeItemWnd | 19 | 14 | 5 | 0 | 10,596 | 98.2% |
| NCTradeWnd | 23 | 18 | 5 | 0 | 3,413 | 94.4% |
| NCTrainInfoWnd | 16 | 11 | 5 | 0 | 4,081 | 95.3% |
| NCTrainListBox | 17 | 12 | 5 | 0 | 2,802 | 93.2% |
| NCTrainListWnd | 13 | 8 | 5 | 0 | 1,238 | 84.6% |
| NCTrainWnd | 16 | 11 | 5 | 0 | 1,144 | 83.3% |
| NCTutorialHtmlViewer | 9 | 4 | 5 | 0 | 356 | 46.3% |
| NCTutorialQuestionWnd | 12 | 7 | 5 | 0 | 680 | 71.9% |
| NCTutorialViewerWnd | 10 | 5 | 5 | 0 | 870 | 78.0% |
| NCVideoOptionWnd | 17 | 12 | 5 | 0 | 12,542 | 98.5% |
| NCVIPShopItemInfoWnd | 11 | 6 | 5 | 0 | 6,956 | 97.3% |
| NCVIPShopItemListBox | 10 | 5 | 5 | 0 | 1,261 | 84.9% |
| NCVIPShopItemListWnd | 14 | 9 | 5 | 0 | 965 | 80.2% |
| NCVIPShopNeededItemWnd | 11 | 6 | 5 | 0 | 1,015 | 81.2% |
| NCVIPShopWnd | 20 | 15 | 5 | 0 | 3,358 | 94.3% |
| NCVirtualWndMain | 31 | 26 | 5 | 0 | 5,223 | 96.3% |
| NCWarningWnd | 17 | 12 | 5 | 0 | 1,408 | 86.4% |
| NCWnd | 49 | 44 | 5 | 0 | 5,669 | 96.6% |
| NDialogInfo | 6 | 6 | 0 | 0 | 381 | 100.0% |
| NHtmlAnalyzer | 8 | 3 | 5 | 0 | 564 | 66.1% |
| NHtmlBlock | 14 | 14 | 0 | 0 | 2,199 | 100.0% |
| NHtmlColorRangeArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| NHtmlItem | 9 | 9 | 0 | 0 | 3,674 | 100.0% |
| NHtmlObject | 5 | 4 | 1 | 0 | 297 | 62.6% |
| NHtmlParser | 20 | 19 | 1 | 0 | 7,318 | 99.5% |
| NHtmlParserBase | 2 | 1 | 1 | 0 | 42 | 16.7% |
| NHtmlTable | 7 | 7 | 0 | 0 | 2,181 | 100.0% |
| NHtmlTag | 4 | 4 | 0 | 0 | 441 | 100.0% |
| NHtmlTagList | 3 | 3 | 0 | 0 | 747 | 100.0% |
| NHtmlTagTable | 5 | 4 | 1 | 0 | 251 | 86.5% |
| NItemEntry | 14 | 14 | 0 | 0 | 1,928 | 100.0% |
| NItemInfo | 1 | 1 | 0 | 0 | 273 | 100.0% |
| NItemRequest | 1 | 1 | 0 | 0 | 52 | 100.0% |
| NL2Font | 11 | 6 | 5 | 0 | 1,058 | 81.1% |
| NLineList | 7 | 7 | 0 | 0 | 1,277 | 100.0% |
| NLineNode | 2 | 2 | 0 | 0 | 152 | 100.0% |
| NListCell | 6 | 6 | 0 | 0 | 178 | 100.0% |
| NListColumnInfo | 4 | 4 | 0 | 0 | 217 | 100.0% |
| NListRow | 5 | 5 | 0 | 0 | 262 | 100.0% |
| NPointDeque | 5 | 5 | 0 | 0 | 670 | 100.0% |
| NShortCutItem | 8 | 8 | 0 | 0 | 651 | 100.0% |
| NSiegeCastleInfo | 1 | 1 | 0 | 0 | 134 | 100.0% |
| NSiegeClanInfo | 1 | 1 | 0 | 0 | 136 | 100.0% |
| NSimPawnVector | 3 | 3 | 0 | 0 | 405 | 100.0% |
| NStdLengthError | 3 | 2 | 1 | 0 | 70 | 51.4% |
| NStdLogicError | 5 | 4 | 1 | 0 | 359 | 90.5% |
| NStdString | 7 | 7 | 0 | 0 | 897 | 100.0% |
| NStlCharAllocator | 1 | 1 | 0 | 0 | 29 | 100.0% |
| NStlVectorBase | 1 | 1 | 0 | 0 | 122 | 100.0% |
| NStlWordVector | 5 | 5 | 0 | 0 | 883 | 100.0% |
| NStlWString | 17 | 17 | 0 | 0 | 4,334 | 100.0% |
| NStlWStringAllocator | 1 | 1 | 0 | 0 | 48 | 100.0% |
| NStlWStringVector | 8 | 8 | 0 | 0 | 1,319 | 100.0% |
| NString | 8 | 8 | 0 | 0 | 454 | 100.0% |
| NStringArray | 4 | 4 | 0 | 0 | 549 | 100.0% |
| NSysMsgParam | 2 | 2 | 0 | 0 | 179 | 100.0% |
| NTrainSkillInfo | 2 | 2 | 0 | 0 | 247 | 100.0% |
| NVIPShopRecord | 1 | 1 | 0 | 0 | 128 | 100.0% |
| TActionItemArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TActorArray | 1 | 1 | 0 | 0 | 28 | 100.0% |
| TArray<FAbnormalStatus> | 1 | 1 | 0 | 0 | 147 | 100.0% |
| TArray<FMinimapTarget> | 2 | 2 | 0 | 0 | 262 | 100.0% |
| TArray<NItemEntry*> | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TArray<NQuestEntry> | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TArray<ptr> | 3 | 3 | 0 | 0 | 396 | 100.0% |
| TChatBalloonArray | 2 | 2 | 0 | 0 | 231 | 100.0% |
| TControlArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TCoolTimeArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TImeTextArray | 3 | 3 | 0 | 0 | 220 | 100.0% |
| TIntArray | 2 | 2 | 0 | 0 | 65 | 100.0% |
| TListItemArray | 2 | 2 | 0 | 0 | 262 | 100.0% |
| TNameArray | 1 | 1 | 0 | 0 | 28 | 100.0% |
| TRadioButtonArray | 2 | 2 | 0 | 0 | 278 | 100.0% |
| TSiegeClanArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TStringPtrArray | 2 | 2 | 0 | 0 | 149 | 100.0% |
| TSysMsgParamArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TTimerArray | 2 | 2 | 0 | 0 | 262 | 100.0% |
| TTrainEntryArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TViewItemMap | 2 | 2 | 0 | 0 | 204 | 100.0% |
| TViewItemMapIterator | 1 | 1 | 0 | 0 | 50 | 100.0% |
| TVIPNeededItemArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| TWndEntryArray | 1 | 1 | 0 | 0 | 144 | 100.0% |
| (sem classe) | 35 | 24 | 11 | 0 | 1,596 | 83.8% |
