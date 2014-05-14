import System.Posix.User as User
import Text.Printf


data Response = Empty String | Error String | ValidString String deriving (Show)

parseInput s = let {
    rev = reverse s;
  } in case rev of
            "" -> Error ""
            ('\n':'\r':"") -> Empty ""
            ('\n':'\r':s') -> ValidString (reverse s')
            _ -> Error ""

usr x = User.getUserEntryForName x

xfmt :: User.UserEntry -> String
xfmt (User.UserEntry userName userPassword userID userGroupID userGecos homeDirectory userShell) = let {
    gecos = head $ wordsWhen (==',') userGecos;
  } in "Login: " ++ printf "%-33s" userName ++ " Name: " ++ gecos
  ++ printf "\nDirectory: %-29s" homeDirectory ++ " Shell: " ++ userShell

output u = printf "%s\n" $ xfmt u

wordsWhen :: (Char -> Bool) -> String -> [String]
wordsWhen p s = case dropWhile p s of
                     "" -> []
                     s' -> w : wordsWhen p s''
                           where (w, s'') = break p s'

sysUser :: User.UserEntry -> Bool
sysUser (User.UserEntry _ _ uid _ _ _ _) = case uid of
                                        x | x < 1000 || x >= 65534  -> True
                                        _ -> False
