import { Navigate, Route, Routes } from 'react-router'
import PublicLayout from './layouts/PublicLayout'
import BackendAdminPage from './pages/BackendAdminPage'
import FaqPage from './pages/FaqPage'
import HomePage from './pages/HomePage'
import NotFoundPage from './pages/NotFoundPage'
import ArticleEditorPage from './pages/ArticleEditorPage'

// 公開頁面共用頂部導覽；後台暫時獨立且沒有身分驗證。
function AppRouter() {
    return (
        <Routes>
            <Route element={<PublicLayout />}>
                <Route index element={<HomePage />} />
                <Route path="games" element={<HomePage eyebrow="GAMES" title="遊戲" description="遊戲內容準備中。" />} />
                <Route path="faq" element={<Navigate to="/faq/getting-started" replace />} />
                <Route path="faq/:articleSlug" element={<FaqPage />} />
                <Route path="about" element={<HomePage eyebrow="ABOUT" title="關於" description="認識三八阿花吹喇叭。" />} />
                <Route path="login" element={<HomePage eyebrow="SIGN IN" title="登入" description="登入功能準備中。" />} />
                {import.meta.env.DEV && <Route path="editor" element={<ArticleEditorPage />} />}
                <Route path="*" element={<NotFoundPage />} />
            </Route>
            <Route path="/admin" element={<BackendAdminPage />} />
        </Routes>
    )
}

export default AppRouter
