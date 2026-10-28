import type { CSSProperties } from 'react'
import { Outlet, useLocation } from 'react-router'
import NavigationMenu from '../components/NavigationMenu'

const pageBackgrounds: Record<string, string> = {
    '/': '#7c878e',
    '/games': '#697b83',
    '/guide': '#78868d',
    '/faq': '#78868d',
    '/about': '#807a85',
    '/login': '#6f8279',
}

function getPageBackground(pathname: string) {
    const matchingPath = Object.keys(pageBackgrounds)
        .filter((path) => path === '/' || pathname === path || pathname.startsWith(`${path}/`))
        .sort((a, b) => b.length - a.length)[0]

    return pageBackgrounds[matchingPath ?? '/']
}

function PublicLayout() {
    const { pathname } = useLocation()
    const pageStyle = { '--page-background': getPageBackground(pathname) } as CSSProperties

    return (
        <div style={pageStyle} className="min-h-svh bg-[var(--page-background)] transition-colors duration-300">
            <NavigationMenu />
            <main>
                <Outlet />
            </main>
        </div>
    )
}

export default PublicLayout
