import { useState } from 'react'
import AdminSidebar from '../components/AdminSidebar'

function BackendAdminPage() {
    const [activeSection, setActiveSection] = useState('devices')

    return (
        <div className="min-h-svh bg-[#f2f2f7] p-4 sm:p-6">
            <div className="mx-auto flex min-h-[calc(100svh-2rem)] max-w-6xl flex-col gap-4 md:min-h-[calc(100svh-3rem)] md:flex-row md:gap-6">
                <AdminSidebar
                    activeSection={activeSection}
                    onSectionChange={setActiveSection}
                />

                <main
                    aria-label={`${activeSection} 內容區`}
                    className="min-h-72 flex-1 rounded-[28px] border border-white/80 bg-white/70 shadow-[0_12px_32px_rgba(60,60,67,0.08)] backdrop-blur-sm"
                />
            </div>
        </div>
    )
}

export default BackendAdminPage
