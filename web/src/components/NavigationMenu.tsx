import { useEffect, useRef, useState } from 'react'
import { BookOpen, Gamepad2, Info, LogIn, Search, X } from 'lucide-react'
import { Link, NavLink } from 'react-router'

const navigationItems = [
    { label: '遊戲', to: '/games', icon: Gamepad2 },
    { label: '指南', to: '/faq', icon: BookOpen },
    { label: '關於', to: '/about', icon: Info },
] as const

/** The primary, horizontal site navigation. */
export default function NavigationMenu() {
    const [hasScrolled, setHasScrolled] = useState(false)
    const [isSearchOpen, setIsSearchOpen] = useState(false)
    const searchInputRef = useRef<HTMLInputElement>(null)

    useEffect(() => {
        const updateScrolledState = () => setHasScrolled(window.scrollY > 0)
        updateScrolledState()
        window.addEventListener('scroll', updateScrolledState, { passive: true })
        return () => window.removeEventListener('scroll', updateScrolledState)
    }, [])

    function openSearch() {
        setIsSearchOpen(true)
        window.setTimeout(() => searchInputRef.current?.focus(), 0)
    }

    function closeSearch() {
        setIsSearchOpen(false)
    }

    return (
        <header
            className={`sticky top-0 z-50 border-b bg-[var(--page-background)] text-white transition-colors duration-200 ${
                hasScrolled ? 'border-white/55' : 'border-transparent'
            }`}
        >
            <div className="mx-auto flex max-w-7xl flex-wrap items-center gap-x-4 gap-y-2 px-1 py-3 sm:px-3">
                <Link
                    to="/"
                    className="shrink-0 rounded-md px-1 py-1 font-black tracking-tight text-white 
                            transition-opacity hover:opacity-85 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-white"
                >
                    <span className="text-2xl sm:text-3xl">三八阿花</span>
                    <span className="ml-1 text-sm text-white/75 sm:text-base">吹喇叭</span>
                </Link>

                <nav aria-label="主要導覽" className="ml-auto flex items-center gap-1 sm:gap-2">
            
                {isSearchOpen ? (
                    <form
                        role="search"
                        onSubmit={(event) => event.preventDefault()}
                        className="flex items-center rounded-full border border-white/80 bg-white/10 px-2 py-1"
                    >
                        <Search size={18} aria-hidden="true" className="shrink-0" />
                        <input
                            ref={searchInputRef}
                            type="search"
                            aria-label="搜尋網站"
                            placeholder="搜尋"
                            className="w-24 bg-transparent px-2 py-1 text-sm font-bold text-white outline-none placeholder:text-white/70 sm:w-44"
                        />
                        <button
                            type="button"
                            onClick={closeSearch}
                            className="rounded-full p-1 transition-colors hover:bg-white/15
                                    focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-white"
                            aria-label="關閉搜尋"
                        >
                            <X size={17} aria-hidden="true" />
                        </button>
                    </form>
                ) : (
                    <button
                        type="button"
                        onClick={openSearch}
                        className="rounded-full p-2 transition-colors hover:bg-white/15
                                focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-white"
                        aria-label="開啟搜尋"
                    >
                        <Search size={21} aria-hidden="true" />
                    </button>
                )}

                {navigationItems.map(({ label, to, icon: Icon }) => (
                    <NavLink
                        key={to}
                        to={to}
                        className={({ isActive }) => `inline-flex items-center gap-1.5
                                rounded-full px-5 py-2 text-xl font-black tracking-tight transition-colors
                                focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-white sm:text-xl ${
                            isActive ? 'bg-white/18 text-white' : 'text-white/90 hover:bg-white/12 hover:text-white'
                        }`}
                    >
                        <Icon size={25} strokeWidth={2.8} aria-hidden="true" />
                        <span>{label}</span>
                    </NavLink>
                ))}

                <NavLink
                    to="/login"
                    className={({ isActive }) => `inline-flex items-center gap-1.5 rounded-full bg-emerald-400 px-5 py-2
                            text-sm font-black tracking-tight text-emerald-950 shadow-md
                            transition-transform hover:-translate-y-px hover:bg-emerald-300
                            focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-white sm:text-xl ${
                        isActive ? 'ring-2 ring-white/90 ring-offset-2 ring-offset-transparent' : ''
                    }`}
                >
                    <LogIn size={18} strokeWidth={2.8} aria-hidden="true" />
                    <span>登入</span>
                </NavLink>
                    </nav>
            </div>
        </header>
    )
}
