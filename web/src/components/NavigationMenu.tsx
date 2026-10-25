import { useState } from 'react'
import { Link } from 'react-router'

function NavigationMenu() {
    const [open, setOpen] = useState(false)

    return (
        <div 
            className="relative" 
            onKeyDown={(event) => { if (event.key === 'Escape') setOpen(false) }}
        >
            <button 
                type="button" 
                aria-expanded={open} 
                aria-controls="site-navigation" 
                onClick={() => setOpen(!open)} 
                className="text-white rounded-lg py-2 px-3 text-sm hover:bg-slate-100 heover:text-slate-900 "
            >
                導覽 
                <span aria-hidden="true">▾</span>
            </button>

            {open && (
                <nav 
                    id="site-navigation" 
                    aria-label="網站導覽" 
                    className="absolute left-0 top-full z-10 mt-2 w-44 rounded-xl border border-slate-200 bg-white p-2 shadow-lg"
                >
                    <Link to="/faq" onClick={() => setOpen(false)} className="block rounded-lg px-3 py-2 hover:bg-slate-100">
                        用戶指南
                    </Link>
                    <span className="block px-3 py-2 text-sm text-slate-400">
                        產品介紹（尚未開放）
                    </span>
                    <span className="block px-3 py-2 text-sm text-slate-400">
                        關於我們（尚未開放）
                    </span>
                </nav>
            )}
        </div>
    )
}

export default NavigationMenu
