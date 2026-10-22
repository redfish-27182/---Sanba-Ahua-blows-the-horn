// 第一版暫用示範內容；未來可替換為從後端取得的資料。
export type FaqArticleData = {
    slug: string // 文章網址 slug
    title: string // 文章標題
    description: string // 文章簡介
    sections: { heading: string; paragraphs: string[] }[] // 文章內容區塊，包含標題與段落
}

export const faqArticles: FaqArticleData[] = [
    {
        slug: 'getting-started',
        title: '開始使用',
        description: '歡迎來到三八阿花使用者指南。這裡的內容目前是示範資料。',
        sections: [
            { heading: '認識使用者指南', paragraphs: ['使用左側列表選擇分類與文章。每篇文章都有獨立網址，方便收藏或分享。'] },
            { heading: '接下來', paragraphs: ['設備說明與操作步驟將在後續版本補上。'] },
        ],
    },
    {
        slug: 'wifi-setup',
        title: '連接 Wi-Fi',
        description: '這是一篇網路設定的示範文章。',
        sections: [{ heading: '網路設定', paragraphs: ['實際的連線步驟與畫面說明將在後續補上。'] }],
    },
    {
        slug: 'device-operation',
        title: '設備操作',
        description: '這是一篇設備操作的示範文章。',
        sections: [{ heading: '基本操作', paragraphs: ['設備的操作方式將在後續補上。'] }],
    },
    {
        slug: 'connection-help',
        title: '連線問題',
        description: '這是一篇常見問題的示範文章。',
        sections: [{ heading: '遇到連線問題時', paragraphs: ['問題排除資訊將在後續補上。'] }],
    },
]
