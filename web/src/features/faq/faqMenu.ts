import type { SidebarMenuItem } from '../../components/SidebarMenu'

// 選單資料與共用列表分開；分類可以有子項，也可以直接連到文章。
export const faqMenu: SidebarMenuItem[] = [
    { label: '開始使用', to: '/faq/getting-started' },
    {
        label: '設備設定',
        children: [
            { label: '連接 Wi-Fi', to: '/faq/wifi-setup' },
            { label: '設備操作', to: '/faq/device-operation' },
        ],
    },
    {
        label: '常見問題',
        children: [
            { label: '連線問題', to: '/faq/connection-help' },
        ],
    },
]
