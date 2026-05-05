from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..database import get_db
from ..models import Task
from ..schemas import TaskCreate, TaskResponse

router = APIRouter(prefix="/tasks", tags=["tasks"])


@router.post("", response_model=TaskResponse, status_code=201)
async def create_task(
    data: TaskCreate, db: AsyncSession = Depends(get_db)
) -> TaskResponse:
    """Create a new task."""
    task = Task(title=data.title, description=data.description)
    db.add(task)
    await db.commit()
    await db.refresh(task)
    return TaskResponse.model_validate(task)


@router.get("", response_model=list[TaskResponse])
async def list_tasks(db: AsyncSession = Depends(get_db)) -> list[TaskResponse]:
    """Return all tasks ordered by creation date descending."""
    result = await db.execute(select(Task).order_by(Task.created_at.desc()))
    return [TaskResponse.model_validate(t) for t in result.scalars().all()]


@router.patch("/{task_id}/complete", response_model=TaskResponse)
async def complete_task(
    task_id: int, db: AsyncSession = Depends(get_db)
) -> TaskResponse:
    """Mark a task as completed."""
    task = await db.get(Task, task_id)
    if task is None:
        raise HTTPException(status_code=404, detail="Task not found")
    task.completed = True
    await db.commit()
    await db.refresh(task)
    return TaskResponse.model_validate(task)


@router.delete("/{task_id}", status_code=204)
async def delete_task(task_id: int, db: AsyncSession = Depends(get_db)) -> None:
    """Delete a task by ID."""
    task = await db.get(Task, task_id)
    if task is None:
        raise HTTPException(status_code=404, detail="Task not found")
    await db.delete(task)
    await db.commit()
