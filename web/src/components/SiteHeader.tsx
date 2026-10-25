import { Link } from 'react-router'
import NavigationMenu from './NavigationMenu'

export default function SiteHeader() {
    return (
        <header className=" bg-black">
            <div className="mx-auto flex max-w-6xl items-center gap-4 px-4 py-4 sm:px-6">
                
                <Link to="/" className=" text-white">
                    <span className="font-bold text-2xl sm:text-3xl px-1">
                        三八阿花
                    </span>  
                    <span className="text-sm sm:text-xl text-slate-400">
                        官方網站
                    </span>
                </Link>
                
                <NavigationMenu />
                
                <button 
                    type="button" disabled 
                    className="ml-auto cursor-not-allowed rounded-lg bg-white px-4 py-2 text-sm text-slate-500" title="登入功能尚未開放"
                >
                    登入（假按鈕）
                </button>
            </div>
        </header>
    )
}

