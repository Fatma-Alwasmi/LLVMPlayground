; ModuleID = 'pointer2.c'
source_filename = "pointer2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @f(i32 noundef %arg) #0 {
entry:
  %arg.addr = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %x = alloca i32*, align 8
  %z = alloca i32, align 4
  store i32 %arg, i32* %arg.addr, align 4
  store i32 0, i32* %a, align 4
  store i32 1, i32* %b, align 4
  %0 = load i32, i32* %arg.addr, align 4
  %tobool = icmp ne i32 %0, 0
  br i1 %tobool, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  store i32* %a, i32** %x, align 8
  br label %if.end

if.else:                                          ; preds = %entry
  store i32* %b, i32** %x, align 8
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %1 = load i32*, i32** %x, align 8
  %2 = load i32, i32* %1, align 4
  %div = sdiv i32 1, %2
  store i32 %div, i32* %z, align 4
  ret i32 0
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
