# This is -*- Python -*-

import poeticunit

class Action:

    def __init__(self, target_poem, target_unit, solutions, is_left=True):
        self.__poem  = target_poem
        self.__i     = target_unit
        self.__solns = solutions
        self.__left  = is_left

        self.__bind_soln = None
        self.__bind_site = -1

 
    def has_solution(self):
        return len(self.__solns) > 0


    def pop_solution(self):
        assert self.has_solution()
        return self.__solns.pop(0)


    def apply(self):
        assert self.__bind_soln is None
        
        assert self.has_solution()
        soln = self.pop_solution()

        self.__bind_site = self.__poem.bind(self.__i, soln,
                                            is_left=self.__left)
        self.__bind_soln = soln


    def revert(self):
        assert self.__bind_soln is not None

        u = self.__poem[self.__bind_site]
        assert u.is_bound()
        assert u.get_binding() == self.__bind_soln

        self.__poem.unbind(self.__bind_site)

        self.__bind_soln = None
        self.__bind_site = -1
        

