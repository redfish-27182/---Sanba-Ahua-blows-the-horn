import type { CSSProperties } from 'react'
import { Outlet, useLocation } from 'react-router'
import NavigationMenu from '../components/NavigationMenu'

type PageTheme = {
    background: string
    foreground: string
}

const pageThemes: Record<string, PageTheme> = {
    '/': { background: '#011627', foreground: '#fdfffc' },
    '/games': { background: '#ff9f1c', foreground: '#011627' },
    '/guide': { background: '#ff9f1c', foreground: '#011627' },
    '/faq':   { background: '#7f8b92', foreground: '#fdfffc' },
    '/about': { background: '#2ec4b6', foreground: '#011627' },
    '/login': { background: '#97b269', foreground: '#fdfffc' },
}

function getPageTheme(pathname: string) {
    const matchingPath = Object.keys(pageThemes)
        .filter((path) => path === '/' || pathname === path || pathname.startsWith(`${path}/`))
        .sort((a, b) => b.length - a.length)[0]

    return pageThemes[matchingPath ?? '/']
}

function PublicLayout() {
    const { pathname } = useLocation()
    const pageTheme = getPageTheme(pathname)
    const pageStyle = {
        '--page-background': pageTheme.background,
        '--page-foreground': pageTheme.foreground,
    } as CSSProperties

    return (
        <div 
            style={pageStyle} 
            className="min-h-svh bg-[var(--page-background)] transition-colors duration-300"
        >
            <NavigationMenu />
            <main key={pathname} className="page-transition">
                <Outlet />
            </main>
        </div>
    )
}

export default PublicLayout
