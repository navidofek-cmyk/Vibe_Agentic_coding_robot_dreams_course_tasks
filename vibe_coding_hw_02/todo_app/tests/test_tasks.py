import pytest
from httpx import AsyncClient


@pytest.mark.asyncio
async def test_create_task(client: AsyncClient) -> None:
    response = await client.post("/tasks", json={"title": "Buy groceries"})
    assert response.status_code == 201
    data = response.json()
    assert data["title"] == "Buy groceries"
    assert data["description"] is None
    assert data["completed"] is False
    assert "id" in data
    assert "created_at" in data


@pytest.mark.asyncio
async def test_create_task_with_description(client: AsyncClient) -> None:
    response = await client.post(
        "/tasks", json={"title": "Read book", "description": "Chapter 1-3"}
    )
    assert response.status_code == 201
    assert response.json()["description"] == "Chapter 1-3"


@pytest.mark.asyncio
async def test_create_task_empty_title_fails(client: AsyncClient) -> None:
    response = await client.post("/tasks", json={"title": ""})
    assert response.status_code == 422


@pytest.mark.asyncio
async def test_list_tasks_empty(client: AsyncClient) -> None:
    response = await client.get("/tasks")
    assert response.status_code == 200
    assert response.json() == []


@pytest.mark.asyncio
async def test_list_tasks(client: AsyncClient) -> None:
    await client.post("/tasks", json={"title": "Task A"})
    await client.post("/tasks", json={"title": "Task B"})
    response = await client.get("/tasks")
    assert response.status_code == 200
    assert len(response.json()) == 2


@pytest.mark.asyncio
async def test_complete_task(client: AsyncClient) -> None:
    create = await client.post("/tasks", json={"title": "Finish report"})
    task_id = create.json()["id"]

    response = await client.patch(f"/tasks/{task_id}/complete")
    assert response.status_code == 200
    assert response.json()["completed"] is True


@pytest.mark.asyncio
async def test_complete_nonexistent_task(client: AsyncClient) -> None:
    response = await client.patch("/tasks/9999/complete")
    assert response.status_code == 404


@pytest.mark.asyncio
async def test_delete_task(client: AsyncClient) -> None:
    create = await client.post("/tasks", json={"title": "Delete me"})
    task_id = create.json()["id"]

    response = await client.delete(f"/tasks/{task_id}")
    assert response.status_code == 204

    tasks = await client.get("/tasks")
    assert tasks.json() == []


@pytest.mark.asyncio
async def test_delete_nonexistent_task(client: AsyncClient) -> None:
    response = await client.delete("/tasks/9999")
    assert response.status_code == 404


@pytest.mark.asyncio
async def test_completed_task_stays_in_list(client: AsyncClient) -> None:
    create = await client.post("/tasks", json={"title": "Stay here"})
    task_id = create.json()["id"]
    await client.patch(f"/tasks/{task_id}/complete")

    tasks = await client.get("/tasks")
    assert len(tasks.json()) == 1
    assert tasks.json()[0]["completed"] is True
