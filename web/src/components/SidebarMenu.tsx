import { useEffect, useId, useRef, useState } from 'react'
import { NavLink, useLocation } from 'react-router'
import { ChevronRight } from 'lucide-react'

// 傳入選單的標題、連結及可選的子項目。
export type SidebarMenuItem = {
    label: string
    to?: string
    children?: { label: string; to: string }[]
}

// 傳入選單的標題、連結及可選的子項目。
type SidebarMenuProps = {
    items: SidebarMenuItem[] 
    label: string
}

type SidebarItemProps = {
    item: SidebarMenuItem // 傳入的選單項目
    isOpen: boolean       // 子選單是否展開
    isActive: boolean     // 是否為目前頁面或其子頁面
    submenuId: string     // 子選單的 HTML id
    onToggle: () => void  // 點擊子選單按鈕時切換展開狀態
    onClose: () => void   // 點擊選單外面時收起子選單
}

// 顯示可展開的子選單及其文章連結。
function Submenu({ children, id, isOpen, onClose }: {
    children: NonNullable<SidebarMenuItem['children']>
    id: string
    isOpen: boolean
    onClose: () => void
}) {
    return (
        <ul
            id={id}
            inert={!isOpen}
            className={`overflow-hidden rounded-lg bg-white shadow-md transition-all duration-200 motion-reduce:transition-none sm:absolute sm:left-full sm:top-0 sm:ml-0 sm:w-48 sm:border sm:border-slate-200 ${isOpen ? 'visible max-h-60 translate-x-0 opacity-100' : 'invisible max-h-0 -translate-x-2 opacity-0 sm:max-h-60'}`}
        >
            {children.map((child) => (
                <li key={child.to}>
                    <NavLink to={child.to} onClick={onClose} className={({ isActive }) => `block rounded-lg px-4 py-3 text-sm focus-visible:outline-2 focus-visible:outline-blue-600 ${isActive ? 'bg-blue-50 font-semibold text-blue-700' : 'text-slate-700 hover:bg-slate-100'}`}>
                        {child.label}
                    </NavLink>
                </li>
            ))}
        </ul>
    )
}

// 依項目內容顯示子選單按鈕、直接連結或停用文字。
function SidebarItem({ item, isOpen, isActive, submenuId, onToggle, onClose }: SidebarItemProps) {
    if (item.children?.length) {
        return (
            <li className="relative">

                {/* 子選單按鈕 */}
                <button
                    type="button"
                    aria-expanded={isOpen}
                    aria-controls={submenuId}
                    onClick={onToggle}
                    className={`flex w-full items-center justify-between rounded-lg px-3 py-3 text-left text-sm font-medium transition-colors focus-visible:outline-2 focus-visible:outline-blue-600 ${isActive || isOpen ? 'bg-blue-100 text-blue-800' : 'text-slate-700 hover:bg-slate-200'}`}
                >
                    {item.label}
                    <ChevronRight 
                        size={16} 
                        aria-hidden="true" 
                        className={`transition-transform sm:rotate-0 ${isOpen ? 'rotate-90' : ''}`} 
                    />
                </button>

                {/* 子選單 */}
                <Submenu id={submenuId} isOpen={isOpen} onClose={onClose}>
                    {item.children}
                </Submenu>
            </li>
        )
    }

    return (
        <li className="relative">
            {item.to ? (
                <NavLink 
                    to={item.to} 
                    onClick={onClose} 
                    className={({ isActive }) => `block rounded-lg px-3 py-3 text-sm font-medium focus-visible:outline-2 focus-visible:outline-blue-600 ${isActive ? 'bg-blue-100 text-blue-800' : 'text-slate-700 hover:bg-slate-200'}`}
                >
                    {item.label}
                </NavLink>
            ) : 
            <span className="block px-3 py-3 text-sm text-slate-400">
                {item.label}
            </span>}
        </li>
    )
}

// 建立側邊選單，包含多層級的選項
export default function SidebarMenu({ items, label }: SidebarMenuProps) {
    const [openIndex, setOpenIndex] = useState<number | null>(null)
    const menuRef = useRef<HTMLElement>(null)
    const menuId = useId()
    const { pathname } = useLocation()

    // 點擊選單外面時收起；觸控裝置也不依賴 hover。
    useEffect(() => {
        function closeOutside(event: PointerEvent) {
            if (menuRef.current && !menuRef.current.contains(event.target as Node)) setOpenIndex(null)
        }
        document.addEventListener('pointerdown', closeOutside)
        return () => document.removeEventListener('pointerdown', closeOutside)
    }, [])

    return (
        <nav
            ref={menuRef}
            aria-label={label}
            className="relative z-10 shrink-0 border-b border-slate-200 bg-slate-50 p-4 sm:w-56 sm:border-b-0 sm:border-r"
            onMouseLeave={() => setOpenIndex(null)}
            onBlur={(event) => 
                { if (!event.currentTarget.contains(event.relatedTarget as Node | null)) setOpenIndex(null) }}
            onKeyDown={(event) => { if (event.key === 'Escape') setOpenIndex(null) }}
        >
            <p className="px-3 pb-3 text-xs font-semibold tracking-wider text-slate-500">
                {label}
            </p>


            <ul className="space-y-1">
                {items.map((item, index) => (
                    <SidebarItem
                        key={item.label}
                        item={item}
                        isOpen={openIndex === index}
                        isActive={item.children?.some((child) => child.to === pathname) ?? false}
                        submenuId={`${menuId}-${index}`}
                        onToggle={() => setOpenIndex(openIndex === index ? null : index)}
                        onClose={() => setOpenIndex(null)}
                    />
                ))}
            </ul>
        </nav>
    )
}
