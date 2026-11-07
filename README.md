# M487 Secure Boot：LDROM 入口與 APROM 更新

本範例以專用 LDROM 入口呼叫晶片內建 Mask ROM Secure ISP API，搭配 **[0213 0921] SecureBootTool.exe**，驗證首次寫入 LDROM／APROM，以及更新 APROM 後仍可再次連線的能力。

**完整圖解與操作：[Secure Boot ISP 操作投影片](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/)**。Secure Boot 的開機驗證、SecureBootTool 的授權更新、ICP／金鑰配置及接線細節，集中於投影片。

## 建置

使用 M487JIDAE／NuMaker-PFM-M487 V3.0、Keil uVision 5／Arm Compiler 6.22。分別開啟下列專案，選擇同名 target 後按 **F7（Build）**：

- APROM：[Template](SampleCode/AP/Keil/Template.uvprojx) → `SampleCode/AP/Keil/obj/template.bin`。
- LDROM：[SecureBootLoader](SampleCode/LDROM/Keil/SecureBootLoader.uvprojx) → `SampleCode/LDROM/Keil/obj/secure_boot_loader.bin`。

確認建置零錯誤／零警告，再使用 BIN 更新；F7 不會下載到 MCU。[建置環境與配置](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#build-examples)。

## 快速驗證

1. 以 **APROM／LDROM 均空白的測試板** 開始，依 [ICP 配置](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#icp-setup) 寫入 CONFIG、KPROM 與 SBK，正常目的地為 APROM；完成後 ICP Disconnect。
2. PC 接 **CON1 HSUSB**，以 SecureBootTool 完成 KPROM 認證及 Key Options，首次同時寫入上述 LDROM／APROM。[連線與首次部署](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#first-programming)。
3. RESET／重新上電，確認 APROM 正常執行。
4. 按 **SW2 至少 30 ms**，或在 **UART0 terminal 按數字 1（KEY1）**，重新連線認證。[按鍵與 terminal 設定](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#entry-triggers)。
5. 載入保留更新入口的新版 APROM，Programming **只勾 APROM**；成功後重置，確認新版執行並可再次進入。[後續更新流程](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#repeat-update)。

**注意事項：**

- 日常更新仍使用 LDROM 的 wrapper／USB ISR，維持 LDROM 不變。
- 每版 APROM 均須保留更新入口與 linker 配置。
- 觸發更新需要 APP 可運行。
- 單槽更新不提供斷電回復保證。

錯誤狀態與恢復限制見投影片的 [排查表](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#troubleshooting)。

## GitHub 維護

- `github_push.bat --check`：檢查本專案的公開內容、遠端與 `main` 分支，不提交或推送。
- `github_push.bat`：檢查通過後輸入 `PUSH`，提交並推送本專案。
- 入口使用工作區公用 `push-project.ps1`。
- 公用腳本位置由 `WEB_PUBLISH_ROOT`，或本機 `github_push.local.psd1` 的 `PublisherRoot` 指定。
- 本機設定與工程紀錄不加入 Git。
