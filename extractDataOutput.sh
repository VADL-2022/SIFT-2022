set -v # verbose command printing
set -e # exit on nonzero exit code
missionName=fullscale1
cd dataOutput

fore1="/home/pi/VanderbiltRocketTeam/dataOutput/LOG_20220219-094701.LIS331.csv
/home/pi/VanderbiltRocketTeam/dataOutput/2022_02_19_09_46_59_AM.video_caplog.txt
/home/pi/VanderbiltRocketTeam/dataOutput/2022-02-19_10_38_57_
/home/pi/VanderbiltRocketTeam/dataOutput/2022-02-19_09_47_01_
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-09_46_59.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_32_31.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_32_07.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_31_18.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_31_16.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_27_40.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_20_18.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-09_30_10.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-09_10_42.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_28_52.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_23_00.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-08_22_28.log.csv"
export trimVideosInThisFolder_fore1=2022-02-19_09_47_01_
export okToUseSinceShortVideosInThisFolder_fore1=2022-02-19_10_38_57_
export trimStartingAt_fore1='00:54:25'

fore2="/home/pi/VanderbiltRocketTeam/dataOutput/2022-02-19_09_32_04_
/home/pi/VanderbiltRocketTeam/dataOutput/2022-02-19_09_31_46_
/home/pi/VanderbiltRocketTeam/dataOutput/2022_02_19_09_31_44_AM.video_caplog.txt
/home/pi/VanderbiltRocketTeam/dataOutput/2022_02_19_09_30_57_AM.video_caplog.txt"
export trimVideosInThisFolder_fore2=2022-02-19_09_32_04_
export okToUseSinceShortVideosInThisFolder_fore2=2022-02-19_09_31_46_
export trimStartingAt_fore2='00:42:15'

sift1="/home/pi/VanderbiltRocketTeam/dataOutput/2022-02-19_10_43_54_
/home/pi/VanderbiltRocketTeam/dataOutput/2022-02-19_11_31_58_CST
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-10_43_52.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/temperature_2022_02_19-10_22_02.log.csv
/home/pi/VanderbiltRocketTeam/dataOutput/LOG_20220219_104353.csv
/home/pi/VanderbiltRocketTeam/dataOutput/2022_02_19_10_43_52_AM.siftlog.txt"
export trimVideosInThisFolder_sift1='2022-02-19_10_43_54_'
export okToUseSinceShortVideosInThisFolder_sift1='2022-02-19_11_31_58_CST'
export trimStartingAt_sift1='00:50:00' # hh:mm:ss format

export hostname="$(hostname)"
# https://stackoverflow.com/questions/50537090/in-bash-is-there-a-way-to-expand-variables-twice-in-double-quotes :
echo "${!hostname}" | xargs -i[] -n 1 find [] -exec stat {} \; > ../ctimes.txt
echo "${!hostname}" | xargs -n 1 realpath --relative-to=. | xargs tar --xattrs --acls --preserve-permissions -H posix --use-compress-program='gzip -9' -cf "../files_$(hostname)_$missionName.tar.gz" ../ctimes.txt && echo $?


# Make a copy of IMU and temperature logs in a separate archive:
echo "${!hostname}" | sed -nr 's/^(.*\.csv)$/\1/p' | xargs -n 1 realpath --relative-to=. | xargs tar --xattrs --acls --preserve-permissions -H posix --use-compress-program='gzip -9' -cf "../files_$(hostname)_imuAndTemperatureOnly_$missionName.tar.gz" && echo $?

# Trim the final merged videos:
echo "${!hostname}" | xargs find | sed -nr 's/^(.*'"$(eval echo \$trimVideosInThisFolder_$hostname)"'\/output\.mp4)$/\1/p' | xargs -n 1 | xargs -i[] bash -c 'ffmpeg -ss "$(eval echo \$trimStartingAt_$hostname)" -i [] -c copy "$(dirname "[]")/output_trimmed.mp4"' # FIXME: quotes in filename aren't handled in the dirname call being done


# Make archive for trimmed videos, ok original length videos, and misc logs only
echo "${!hostname}" | xargs find | sed -nr 's/^(.*output_trimmed\.mp4|.*\.txt)$/\1/p' | xargs -n 1 realpath --relative-to=. | xargs tar --xattrs --acls --preserve-permissions -H posix --use-compress-program='gzip -9' -cf "../files_$(hostname)_videosTrimmedOnly_$missionName.tar.gz" ../ctimes.txt "$(eval echo \$okToUseSinceShortVideosInThisFolder_$hostname)" && echo $?


cd ..
mv "./files_$(hostname)_imuAndTemperatureOnly_$missionName.tar.gz"  "./files_$(hostname)_videosTrimmedOnly_$missionName.tar.gz" Data/ # Note: this failed on fore2 since it didn't have any temperature logs for some reason so tar refused to create an empty archive for the `_imuAndTemperatureOnly_` part.. so run the rest of the commands below manually:
cd Data
git add -A
git commit -m "$missionName data added from $(hostname) pi"
git push
