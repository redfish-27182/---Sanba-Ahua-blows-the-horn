import { Outlet } from 'react-router'
import SiteHeader from '../components/SiteHeader'

// 公開網站頁面共用導覽列，Outlet 放入目前路由的內容。
function PublicLayout() {
    return (
        <div className="min-h-svh bg-slate-50 text-slate-900">
            <SiteHeader />
            <main><Outlet /></main>
        </div>
    )
}

export default PublicLayout
