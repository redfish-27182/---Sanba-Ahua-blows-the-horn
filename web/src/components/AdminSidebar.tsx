import { Cpu, Upload, type LucideIcon } from 'lucide-react'

export type AdminSection = 'devices' | 'firmware'

interface AdminSidebarProps {
    activeSection: AdminSection
    onSectionChange: (section: AdminSection) => void
}

interface NavigationItem {
    id: AdminSection
    label: string
    Icon: LucideIcon
    iconClassName: string
}

const navigationItems: NavigationItem[] = [
    {
        id: 'devices',
        label: '設備',
        Icon: Cpu,
        iconClassName: 'bg-blue-500',
    },
    {
        id: 'firmware',
        label: '韌體',
        Icon: Upload,
        iconClassName: 'bg-indigo-500',
    },
]

function AdminSidebar({ activeSection, onSectionChange }: AdminSidebarProps) {
    return (
        <aside className="w-full shrink-0 rounded-[28px] border border-white/80 bg-white/70 p-3 shadow-[0_12px_32px_rgba(60,60,67,0.08)] backdrop-blur-sm md:w-60">
            <nav aria-label="後台功能" className="flex gap-2 md:flex-col">
                {navigationItems.map(({ id, label, Icon, iconClassName }) => {
                    const isActive = activeSection === id

                    return (
                        <button
                            key={id}
                            type="button"
                            aria-pressed={isActive}
                            onClick={() => onSectionChange(id)}
                            className={`flex flex-1 items-center gap-3 rounded-2xl px-3 py-3 text-left text-[15px] font-medium transition-colors focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-500 md:flex-none ${
                                isActive
                                    ? 'bg-blue-500 text-white shadow-sm'
                                    : 'text-[#3a3a3c] hover:bg-black/5'
                            }`}
                        >
                            <span className={`grid size-8 place-items-center rounded-[10px] text-white ${iconClassName}`}>
                                <Icon aria-hidden="true" size={18} strokeWidth={2.25} />
                            </span>
                            {label}
                        </button>
                    )
                })}
            </nav>
        </aside>
    )
}

export default AdminSidebar
