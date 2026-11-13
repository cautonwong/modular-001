from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Iterable

from sim import SimContext


class SelectorError(ValueError):
    pass


@dataclass(frozen=True)
class Case:
    id: str
    clause: str
    tags: tuple[str, ...]
    fn: Callable[[SimContext], None]


_CASES: list[Case] = []


def case(id: str, clause: str, tags: Iterable[str] = ()):
    def register(fn: Callable[[SimContext], None]) -> Callable[[SimContext], None]:
        if any(existing.id == id for existing in _CASES):
            raise SelectorError(f"duplicate acceptance case id {id}")
        _CASES.append(Case(id, clause, ("acceptance", *tags), fn))
        return fn

    return register


def all_cases() -> tuple[Case, ...]:
    return tuple(_CASES)


def select(selectors: Iterable[str]) -> tuple[Case, ...]:
    specs = [part for selector in selectors for part in selector.split(",") if part]
    if not specs:
        return all_cases()

    chosen: list[Case] = []
    for spec in specs:
        if ":" not in spec:
            raise SelectorError(f"selector {spec!r} needs a case: or tag: prefix")
        prefix, value = spec.split(":", 1)
        if prefix == "case":
            matches = [item for item in _CASES if item.id == value]
        elif prefix == "tag":
            matches = [item for item in _CASES if value in item.tags]
        else:
            raise SelectorError(f"unknown selector prefix {prefix!r} in {spec!r}")
        if not matches:
            raise SelectorError(f"selector {spec!r} matched no acceptance cases")
        for item in matches:
            if item not in chosen:
                chosen.append(item)
    return tuple(chosen)


def describe(cases: Iterable[Case]) -> str:
    return "\n".join(
        f"{item.id}\ttags={','.join(item.tags)}\t{item.clause}" for item in cases
    )
