type HomePageProps = {
    eyebrow?: string
    title?: string
    description?: string
}

function HomePage({
    eyebrow,
    title = '三八阿花吹喇叭',
    description = '歡迎來到三八阿花。',
}: HomePageProps) {
    return (
        <section className="grid min-h-[calc(100svh-4.75rem)] place-items-center px-4 text-white">
            <div className="max-w-xl text-center">
                {eyebrow && <p className="mb-3 text-sm font-black tracking-[0.25em] text-white/70">{eyebrow}</p>}
                <h1 className="text-4xl font-black tracking-tight sm:text-6xl">{title}</h1>
                <p className="mt-5 text-base font-bold text-white/80 sm:text-lg">{description}</p>
            </div>
        </section>
    )
}

export default HomePage
