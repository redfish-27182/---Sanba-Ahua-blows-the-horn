"""檢查文章 API 的欄位與基本安全限制。"""
import json
from datetime import datetime
from typing import Any
from urllib.parse import urlsplit

from pydantic import BaseModel, ConfigDict, Field, field_validator


class ArticleInput(BaseModel):
    """儲存文章所需的標題和 Tiptap JSON。"""
    title: str = Field(min_length=1, max_length=200)
    content: dict[str, Any]

    @field_validator("title")
    @classmethod
    def title_not_blank(cls, value: str) -> str:
        """拒絕只有空白的標題。"""
        value = value.strip()
        if not value:
            raise ValueError("標題不能空白")
        return value

    @field_validator("content")
    @classmethod
    def validate_content(cls, value: dict[str, Any]) -> dict[str, Any]:
        """限制文件大小、結構與圖片／連結網址。"""
        if len(json.dumps(value, ensure_ascii=False).encode("utf-8")) > 1_000_000:
            raise ValueError("文章內容超過 1 MB")
        if value.get("type") != "doc":
            raise ValueError("文章內容必須是 Tiptap 文件")
        allowed_nodes = {"doc", "paragraph", "heading", "text", "bulletList", "orderedList", "listItem", "horizontalRule", "hardBreak", "blockquote", "codeBlock", "image"}
        allowed_marks = {"bold", "italic", "strike", "underline", "code", "link"}
        pending = [(value, 0)]
        count = 0
        while pending:
            node, depth = pending.pop()
            count += 1
            if count > 10000 or depth > 32 or not isinstance(node, dict) or node.get("type") not in allowed_nodes:
                raise ValueError("文章結構無效或過於複雜")
            attrs = node.get("attrs") or {}
            if not isinstance(attrs, dict):
                raise ValueError("文章屬性格式錯誤")
            if node.get("type") == "image":
                validate_url(attrs.get("src"))
            marks = node.get("marks", [])
            if not isinstance(marks, list):
                raise ValueError("文字格式錯誤")
            for mark in marks:
                if not isinstance(mark, dict) or mark.get("type") not in allowed_marks:
                    raise ValueError("不支援的文字格式")
                if mark.get("type") == "link":
                    link_attrs = mark.get("attrs") or {}
                    if not isinstance(link_attrs, dict):
                        raise ValueError("連結屬性格式錯誤")
                    validate_url(link_attrs.get("href"))
            children = node.get("content", [])
            if not isinstance(children, list):
                raise ValueError("文章內容格式錯誤")
            pending.extend((child, depth + 1) for child in children)
        return value


def validate_url(value: object) -> None:
    """圖片和連結僅允許完整的 HTTP(S) 網址。"""
    if not isinstance(value, str) or len(value) > 2048:
        raise ValueError("圖片或連結網址無效")
    url = urlsplit(value)
    if url.scheme not in {"http", "https"} or not url.hostname or url.username or url.password:
        raise ValueError("圖片或連結必須使用 HTTP(S) 網址")


class ArticleResponse(ArticleInput):
    """回傳文章與時間資訊。"""
    model_config = ConfigDict(from_attributes=True)
    slug: str
    created_at: datetime
    updated_at: datetime
