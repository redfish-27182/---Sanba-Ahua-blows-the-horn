import { Navigate, Route, Routes } from 'react-router'
import PublicLayout from './layouts/PublicLayout'
import BackendAdminPage from './pages/BackendAdminPage'
import EmptyPage from './pages/EmptyPage'
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
                <Route path="games" element={<EmptyPage />} />
                <Route path="guide" element={<EmptyPage />} />
                <Route path="faq" element={<Navigate to="/faq/getting-started" replace />} />
                <Route path="faq/:articleSlug" element={<FaqPage />} />
                <Route path="about" element={<EmptyPage />} />
                <Route path="login" element={<EmptyPage />} />
                {import.meta.env.DEV && <Route path="editor" element={<ArticleEditorPage />} />}
                <Route path="*" element={<NotFoundPage />} />
            </Route>
            <Route path="/admin" element={<BackendAdminPage />} />
        </Routes>
    )
}

export default AppRouter
