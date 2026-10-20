import { Link } from 'react-router'
import NavigationMenu from './NavigationMenu'

function SiteHeader() {
    return (
        <header className="border-b border-slate-200 bg-white text-slate-900">
            <div className="mx-auto flex max-w-6xl items-center gap-4 px-4 py-4 sm:px-6">
                
                <Link to="/" className="font-bold text-slate-900 hover:bg-blue-200 hover:text-blue-900 rounded-lg px-2 py-1 transition-colors">
                    <span className="text-2xl sm:text-xl px-2">
                        三八阿花
                    </span>
                    
                    <span className="text-2xl sm:text-sm">
                        官方網站
                    </span>

                </Link>

                <NavigationMenu />
                
                <button type="button" disabled className="ml-auto cursor-not-allowed rounded-lg bg-slate-100 px-4 py-2 text-sm text-slate-500" title="登入功能尚未開放">
                    登入（假按鈕）
                </button>
            </div>
        </header>
    )
}

export default SiteHeader
