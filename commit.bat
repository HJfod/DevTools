@echo off

if "%1"=="pull" (

    git pull origin main

    cd sdk
    git pull origin main

    cd bin
    git pull origin main

) else (

    echo ""
    echo "==== geode/bin ===="
    echo ""

    cd sdk/bin

    git add --all
    git commit -a
    git push origin main

    echo ""
    echo "==== geode/sdk ===="
    echo ""

    cd ..

    git add --all
    git commit -a
    git push origin main

    echo "==== hjfod/devtools ===="
    echo ""

    cd ..

    git add --all
    git commit -a
    git push origin main

)
