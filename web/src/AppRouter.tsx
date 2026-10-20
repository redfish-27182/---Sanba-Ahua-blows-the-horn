import { Route, Routes } from 'react-router'
import PublicLayout from './layouts/PublicLayout'
import BackendAdminPage from './pages/BackendAdminPage'
import FaqPage from './pages/FaqPage'
import HomePage from './pages/HomePage'
import NotFoundPage from './pages/NotFoundPage'

// 公開頁面共用頂部導覽；後台暫時獨立且沒有身分驗證。
function AppRouter() {
    return (
        <Routes>
            <Route element={<PublicLayout />}>
                <Route index element={<HomePage />} />
                <Route path="faq" element={<FaqPage />} />
                <Route path="*" element={<NotFoundPage />} />
            </Route>
            <Route path="/admin" element={<BackendAdminPage />} />
        </Routes>
    )
}

export default AppRouter
