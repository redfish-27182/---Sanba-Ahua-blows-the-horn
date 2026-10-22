"""文章 API 測試；使用替身服務，不寫入實際資料庫。"""
import unittest
from unittest.mock import patch

from fastapi.testclient import TestClient

from app.main import app
from app.config.settings import settings


class ArticleApiTests(unittest.TestCase):
    def setUp(self):
        """使用本機測試客戶端，不啟動 MQTT 生命週期。"""
        self.client = TestClient(app, client=("127.0.0.1", 1234))
        self.content = {"type": "doc", "content": [{"type": "paragraph", "content": [{"type": "text", "text": "測試"}]}]}
        self.saved = {"slug": "test", "title": "測試", "content": self.content, "created_at": "2026-01-01T00:00:00Z", "updated_at": "2026-01-01T00:00:00Z"}

    def test_get_missing_article(self):
        """不存在的文章回傳 404。"""
        with patch("app.api.articles.article_service.get_article", return_value=None):
            self.assertEqual(self.client.get("/api/articles/test").status_code, 404)

    def test_write_disabled(self):
        """預設停用寫入，不應呼叫儲存服務。"""
        with patch.object(settings, "article_edit_enabled", False), patch("app.api.articles.article_service.save_article") as save:
            response = self.client.put("/api/articles/test", json={"title": "測試", "content": self.content})
            self.assertEqual(response.status_code, 403)
            save.assert_not_called()

    def test_write_from_local_origin(self):
        """啟用後只接受本機測試來源。"""
        with patch.object(settings, "article_edit_enabled", True), patch("app.api.articles.article_service.save_article", return_value=self.saved):
            response = self.client.put("/api/articles/test", headers={"Origin": "http://localhost:5173"}, json={"title": "測試", "content": self.content})
            self.assertEqual(response.status_code, 200)
            self.assertEqual(response.json()["content"], self.content)
            response = self.client.put("/api/articles/test", headers={"Origin": "https://example.com"}, json={"title": "測試", "content": self.content})
            self.assertEqual(response.status_code, 403)

    def test_invalid_image_url(self):
        """拒絕不是 HTTP(S) 的圖片來源。"""
        content = {"type": "doc", "content": [{"type": "image", "attrs": {"src": "javascript:alert(1)"}}]}
        with patch.object(settings, "article_edit_enabled", True):
            response = self.client.put("/api/articles/test", json={"title": "測試", "content": content})
            self.assertEqual(response.status_code, 422)


if __name__ == "__main__":
    unittest.main()
