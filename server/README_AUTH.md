# Email/password authentication

目前支援 Email 與密碼註冊登入；Google、GitHub、Email 驗證與忘記密碼尚未啟用。

## Local setup

1. 將 `.env.example` 的 session 設定補到 `server/.env`。
2. 本機維持 `AUTH_COOKIE_SECURE=false`。
3. 執行 `alembic upgrade head` 建立使用者、密碼憑證與 session 資料表。
4. 啟動 FastAPI，再從前端 `/register` 註冊第一個一般使用者。

密碼只會以 Argon2 雜湊值儲存。要建立第一個管理員，請在可信任的資料庫管理流程中把該使用者的 `role` 改為 `admin`；公開註冊 API 不接受角色欄位。

## Production

- 設定 `FRONTEND_URL` 與 `CORS_ORIGINS` 為正式前端網址。
- 設定 `AUTH_COOKIE_SECURE=true`。
- 前端與後端同網域代理完成前，不要把登入功能公開給外部使用者。
