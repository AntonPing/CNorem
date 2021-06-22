module NoremRuntime (module NoremRuntime) where
import Prelude
import Text.Parsec.Prim
import Text.Parsec.Combinator
import Debug.Trace


{-
class Gen a where
    gen :: a -> String
instance Gen Term where
    gen (Var x) = x
    gen (Abs x t) = undefined
    gen (App t1 t2) = "App(" ++ gen t1 ++ "," ++ gen t2 ++ ")"
    gen I = "Ic"
    gen K = "Kc"
    gen S = "Sc"
    gen B = "Bc"
    gen C = "Cc"
    gen S' = "Sp"
    gen B' = "Bp"
    gen C' = "Cp"
-}


data Term =
      Var String
    | Abs String Term
    | App Term Term
    | I | K | S | B | C 
    | S' | B' | C'
    deriving (Eq)

instance Show Term where
    show (Var x) = x
    show (Abs x t) = "λ." ++ x ++ " " ++ show t
    show (App t1 t2) = "(" ++ show t1 ++ " " ++ show t2 ++ ")"
    show I = "I"
    show K = "K"
    show S = "S"
    show B = "B"
    show C = "C"
    show S' = "S'"
    show B' = "B*"
    show C' = "C'"

isPureLamb :: Term -> Bool
isPureLamb (Var x) = True 
isPureLamb (Abs x t) = isPureLamb t
isPureLamb (App t1 t2) =
    isPureLamb t1 && isPureLamb t2
isPureLamb _ = False

isPureComb :: Term -> Bool 
isPureComb (Var x) = False 
isPureComb (Abs x t) = False
isPureComb (App t1 t2) =
    isPureComb t1 && isPureComb t2
isPureComb _ = True 

{-
normal :: Term -> Bool
normal (Var _) = True
normal (Abs _ _) = True
normal (App (Var _) _) = True
normal (App (Abs _ _) _) = False
normal (App t@(App _ _) _) = normal t
normal (App t@(App _ _) _) = normal t
-}

isFree :: String -> Term -> Bool
isFree v (Var x) = x /= v
isFree v (Abs x t) = x == v || isFree v t
isFree v (App t1 t2) = isFree v t1 && isFree v t2
isFree v combs = True

compile :: Term -> Term
compile (Var x) = Var x
compile (Abs x t)
    | isFree x t = App K t
compile (Abs x (Var y)) = -- x /= y since x is not free
    I
compile (Abs x (Abs y t)) =
    Abs x $ compile (Abs y t)
compile (Abs x (App t1 t2)) =
    App (App S (Abs x t1)) (Abs x t2)     
compile (Abs x combs) = App K combs
compile (App t1 t2) =
    if isPureComb t1
    then App t1 (compile t2)
    else App (compile t1) t2
compile combs = combs


reduceComb :: Term -> Term
reduceComb (App I x) = x
reduceComb (App (App K c) x) = c
reduceComb (App (App (App S f) g) x) =
    App (App f x) (App g x)
reduceComb (App (App (App C f) g) x) =
    App (App f x) g
reduceComb (App (App (App B f) g) x) =
    App f (App g x)
reduceComb (App t1 t2) =
    let t1' = reduceComb t1
        t2' = reduceComb t2 in
    if t1' == t1 then App t1 t2' else App t1' t2
reduceComb lambs = lambs


optComb :: Term -> Term
optComb (App (App S (App K p)) (App K q)) =
    App K (App p q)
optComb (App (App S (App K p)) I) = p
optComb (App (App S (App K p)) (App (App B q) r)) =
    App (App (App B' p) q) r
optComb (App (App S (App K p)) q) =
    App (App B p) q
optComb (App (App S (App (App B p) q)) (App K r)) =
    App (App (App C' p) q) r
optComb (App (App S p) (App K q)) =
    App (App C p) q
optComb (App (App (App B p) q) r) =
    App (App (App S' p) q) r
optComb (App t1 t2) =
    let t1' = optComb t1
        t2' = optComb t2 in
    if t1' == t1 then App t1 t2' else App t1' t2
optComb other = other


iterTrace :: Show a => Eq a => (a -> a) -> a -> a
iterTrace f x = trace (show x)
    (if f x == x then x else iterTrace f (f x))


test = Abs "x" (Abs "y" (App (Var "y") (Var "x")))

testComb = iterTrace compile test

testOpt = iterTrace optComb testComb

result = iterTrace reduceComb $ App (App testComb (Var "1")) (Var "2")