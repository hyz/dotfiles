
### https://stackoverflow.com/questions/4799905/casting-back-to-more-specialised-interface

    package main

    type Entity interface {
        a() string
    }

    type PhysEntity interface {
        Entity
        b() string
    }

    type BaseEntity struct { }
    func (e *BaseEntity) a() string { return "Hello " }

    type BasePhysEntity struct { BaseEntity }
    func (e *BasePhysEntity) b() string { return " World!" }

    func main() {
        physEnt := PhysEntity(new(BasePhysEntity))
        entity := Entity(physEnt)
        print(entity.a())
        original := PhysEntity(entity)
    // ERROR on line above: cannot convert physEnt (type PhysEntity) to type Entity:
        println(original.b())
    }

    ------------------------

    original, ok := entity.(PhysEntity)
    if ok {
        println(original.b())
    }

