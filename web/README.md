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
| `/faq` | 使用者指南，導向第一篇文章。 |
| `/faq/:slug` | 從後端讀取文章。 |
| `/editor` | 只在 Vite 開發模式提供的暫時編輯頁。 |
| `/admin` | 既有後台控制介面，目前沒有身分驗證。 |

**`/admin` 公開可存取，不可當作已受保護的管理介面。** 正式提供管理功能前，需在後端實作身分驗證和權限檢查。

## 後端連線

Axios 的共用設定放在 `src/api/client.ts`，開發模式預設連至 `http://127.0.0.1:8000`。可用 `VITE_API_BASE_URL` 設定其他後端網址；所有 `VITE_` 變數會進入前端程式，不能放密碼或金鑰。後端的 CORS 目前只允許本機開發來源；線上部署需另行設定受信任的前端來源。

## 本機文章測試

後端原本的 `.venv` 若仍可用，可以使用它；若出現 `No Python at ...`，請先建立新的虛擬環境（不會刪除舊的 `.venv`）：

```powershell
cd server
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m venv .venv-articles
$env:PYTHONUTF8 = '1'
.\.venv-articles\Scripts\python.exe -m pip install -r requirements.txt
```

在後端套用 migration，並只在本機測試時啟用寫入：

```powershell
$env:PYTHONUTF8 = '1'
.\.venv-articles\Scripts\python.exe -m alembic upgrade head
$env:ARTICLE_EDIT_ENABLED = 'true'
.\.venv-articles\Scripts\python.exe -m uvicorn app.main:app --host 127.0.0.1 --port 8000 --reload
```

再於另一個終端機啟動前端，開啟 `http://localhost:5173/editor`，使用 `getting-started` 作為網址名稱並儲存，然後開啟 `/faq` 查看。左側選單目前是固定資料，不會隨文章自動增加。圖片使用公開的 HTTP(S) 網址；請確認授權和外站引用政策。

寫入功能預設關閉，開啟後仍**沒有使用者驗證**；不要在公開或有反向代理的環境啟用。正式線上編輯前必須實作後端身分驗證與授權。瀏覽器開發工具或其他本機程式都可能呼叫開啟的寫入 API。

## Vercel 部署

在 Vercel 匯入此 Git 儲存庫時，把 **Root Directory** 設成 `web`，框架選擇 Vite，建置指令使用 `npm run build`，輸出目錄使用 `dist`。`vercel.json` 已設定單頁應用路由，供直接開啟或重新整理 `/faq`、`/admin` 使用。

Vercel 只會部署已提交並推送到所連結 Git 儲存庫的版本；本機未提交的修改需要先提交和推送。發布後網站和沒有驗證的 `/admin` 都可能被網路上的使用者存取。
