# 官方網站

前端使用 React、TypeScript、Vite、Tailwind CSS 和 React Router。

## 本地開發

在 `web/` 目錄執行：

```powershell
npm install
npm run dev
```

其他檢查：

```powershell
npm run typecheck
npm run build
```

## 路由

| 網址 | 功能 |
| --- | --- |
| `/` | 顯示「網站施工中」。 |
| `/faq` | FAQ 頁面，目前先留白。 |
| `/admin` | 既有後台控制介面，目前沒有身分驗證。 |

**`/admin` 公開可存取，不可當作已受保護的管理介面。** 正式提供管理功能前，需在後端實作身分驗證和權限檢查。

## 後端連線

Axios 的共用設定放在 `src/api/client.ts`，目前不會發送請求。未來可使用 `VITE_API_BASE_URL` 設定 API 網址。所有 `VITE_` 前綴的變數會進入前端程式，不能放密碼或金鑰。

## Vercel 部署

在 Vercel 匯入此 Git 儲存庫時，把 **Root Directory** 設成 `web`，框架選擇 Vite，建置指令使用 `npm run build`，輸出目錄使用 `dist`。`vercel.json` 已設定單頁應用路由，供直接開啟或重新整理 `/faq`、`/admin` 使用。

Vercel 只會部署已提交並推送到所連結 Git 儲存庫的版本；本機未提交的修改需要先提交和推送。發布後網站和沒有驗證的 `/admin` 都可能被網路上的使用者存取。
